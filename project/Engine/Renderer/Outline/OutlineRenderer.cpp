#include "OutlineRenderer.h"

#include <Engine\Assets\Animation\AnimationModel.h>
#include <Engine\Assets\Model\BaseModel.h>
#include <Engine\Graphics\Camera\3d\Camera3d.h>
#include <Engine\Graphics\Pipeline\Pso\PsoDetails.h>
#include <Engine\Graphics\RenderTarget\Interface\IRenderTarget.h>
#include <Engine\Objects\3D\Actor\SceneObject.h>
#include <Engine\PostProcess\FullscreenDrawer.h>

#include <algorithm>

namespace {
	constexpr DXGI_FORMAT kNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	D3D12_RECT MakeRect(uint32_t width, uint32_t height) {
		return D3D12_RECT{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	}
}

void OutlineRenderer::Render(ID3D12GraphicsCommandList* cmdList,
							 ID3D12Device*				device,
							 IRenderTarget*				rt,
							 PipelineService*			psoService,
							 const Camera3d*			camera,
							 const ModelRenderer&		modelRenderer) {
	if(!cmdList || !device || !rt || !psoService || !camera) return;

	const auto viewport = rt->GetViewport();
	EnsureResources(device, viewport);
	compositeConstants_ = CompositeConstants{};
	RenderNormalBuffer(cmdList, device, rt, psoService, camera, modelRenderer);
	Composite(cmdList, rt, psoService);
}

void OutlineRenderer::EnsureResources(ID3D12Device* device, const D3D12_VIEWPORT& viewport) {
	const uint32_t width = static_cast<uint32_t>(viewport.Width);
	const uint32_t height = static_cast<uint32_t>(viewport.Height);
	if(width == 0 || height == 0) return;
	if(width_ == width && height_ == height && normalResource_.Get() && compositeResource_.Get()) return;

	width_ = width;
	height_ = height;

	if(!normalRtv_.IsValid()) {
		normalRtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	}
	if(!compositeRtv_.IsValid()) {
		compositeRtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	}

	normalResource_.InitializeAsRenderTarget(device, width_, height_, kNormalFormat, L"OutlineNormal");
	normalResource_.CreateRTV(device, normalRtv_.cpu);
	if(!normalResource_.GetSRVGpuHandle().ptr) {
		normalResource_.CreateSRV(device);
	} else {
		normalResource_.UpdateSRV(device);
	}
	normalResource_.SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	compositeResource_.InitializeAsRenderTarget(device, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, L"OutlineComposite");
	compositeResource_.CreateRTV(device, compositeRtv_.cpu);
	if(!compositeResource_.GetSRVGpuHandle().ptr) {
		compositeResource_.CreateSRV(device);
	} else {
		compositeResource_.UpdateSRV(device);
	}
	compositeResource_.SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void OutlineRenderer::RenderNormalBuffer(ID3D12GraphicsCommandList* cmdList,
										 ID3D12Device*				 device,
										 IRenderTarget*				 rt,
										 PipelineService*			 psoService,
										 const Camera3d*			 camera,
										 const ModelRenderer&		 modelRenderer) {
	normalResource_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const float clearNormal[] = {0.5f, 0.5f, 0.5f, 0.0f};
	cmdList->ClearRenderTargetView(normalRtv_.cpu, clearNormal, 0, nullptr);

	const auto viewport = rt->GetViewport();
	const auto scissor = MakeRect(width_, height_);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);
	auto dsv = rt->GetDSV();
	cmdList->OMSetRenderTargets(1, &normalRtv_.cpu, FALSE, &dsv);

	std::vector<ModelRenderer::RenderInstance> staticInstances;
	std::vector<ModelRenderer::RenderInstance> skinnedInstances;
	modelRenderer.CollectVisibleStatic(staticInstances);
	modelRenderer.CollectVisibleSkinned(skinnedInstances);
	bool hasOutlineSettings = false;

	auto applyOwnerSettings = [&](const SceneObject& owner) {
		const auto& settings = owner.GetOutlineSettings();
		if(!hasOutlineSettings) {
			compositeConstants_.color = settings.color;
			compositeConstants_.thickness = settings.thickness;
			hasOutlineSettings = true;
		} else {
			compositeConstants_.thickness = (std::max)(compositeConstants_.thickness, settings.thickness);
		}
	};

	std::vector<StaticBatch> staticBatches;
	for(const auto& inst : staticInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(!inst.owner->IsOutlineEnabled()) continue;
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;
		applyOwnerSettings(*inst.owner);

		auto it = std::find_if(staticBatches.begin(), staticBatches.end(),
							   [&](const StaticBatch& batch) { return batch.model == inst.model; });
		if(it == staticBatches.end()) {
			StaticBatch batch;
			batch.model = inst.model;
			staticBatches.emplace_back(std::move(batch));
			it = std::prev(staticBatches.end());
		}

		it->transforms.push_back(*inst.transform);
		GpuBillboardParams billboard{};
		billboard.mode = static_cast<uint32_t>(inst.billboardMode);
		it->billboards.push_back(billboard);
	}

	if(!staticBatches.empty()) {
		const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineNormalObject3D, BlendMode::NONE);
		psoService->SetCommand(ps, cmdList);
		camera->SetRootCommand(cmdList, 4);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for(auto& batch : staticBatches) {
			const UINT count = static_cast<UINT>(batch.transforms.size());
			if(count == 0) continue;

			batch.model->EnsureInstanceCapacity(device, count);
			batch.model->UploadInstanceMatrices(batch.transforms);
			cmdList->SetGraphicsRootDescriptorTable(1, batch.model->GetInstanceSrv());

			batch.model->EnsureBillboardCapacity(device, count);
			batch.model->UploadBillboardParams(batch.billboards);
			cmdList->SetGraphicsRootDescriptorTable(7, batch.model->GetBillboardSrv());

			batch.model->BindVertexIndexBuffers(cmdList);
			cmdList->DrawIndexedInstanced(
				static_cast<UINT>(batch.model->GetModelData()->meshResource.Indices().size()),
				count,
				0,
				0,
				0);
		}
	}

	if(skinnedInstances.empty()) return;

	bool skinnedPipelineSet = false;
	for(const auto& inst : skinnedInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(!inst.owner->IsOutlineEnabled()) continue;
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;
		applyOwnerSettings(*inst.owner);

		if(!skinnedPipelineSet) {
			const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineNormalSkinnedObject3D, BlendMode::NONE);
			psoService->SetCommand(ps, cmdList);
			camera->SetRootCommand(cmdList, 4);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			skinnedPipelineSet = true;
		}

		auto* model = static_cast<CalyxEngine::AnimationModel*>(inst.model);
		model->Draw(*inst.transform);
	}
}

void OutlineRenderer::Composite(ID3D12GraphicsCommandList* cmdList,
								IRenderTarget*				rt,
								PipelineService*			psoService) {
	normalResource_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionDepthTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	compositeResource_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto viewport = rt->GetViewport();
	const auto scissor = MakeRect(width_, height_);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);
	cmdList->OMSetRenderTargets(1, &compositeRtv_.cpu, FALSE, nullptr);

	const auto compositePs = psoService->GetPipelineSet(PipelineTag::PostProcess::OutlineComposite);
	compositePs.SetCommand(cmdList);

	CompositeConstants constants = compositeConstants_;
	constants.texelSize = {1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_)};
	constants.thickness = std::clamp(constants.thickness, 1.0f, 4.0f);

	cmdList->SetGraphicsRootDescriptorTable(0, rt->GetSRV());
	cmdList->SetGraphicsRootDescriptorTable(1, rt->GetDepthSRV());
	cmdList->SetGraphicsRootDescriptorTable(2, normalResource_.GetSRVGpuHandle());
	cmdList->SetGraphicsRoot32BitConstants(3, sizeof(CompositeConstants) / sizeof(uint32_t), &constants, 0);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);

	compositeResource_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	rt->TransitionDepthTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	rt->SetRenderTarget(cmdList);

	const auto copyPs = psoService->GetPipelineSet(PipelineTag::PostProcess::CopyImage);
	copyPs.SetCommand(cmdList);
	cmdList->SetGraphicsRootDescriptorTable(0, compositeResource_.GetSRVGpuHandle());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}
