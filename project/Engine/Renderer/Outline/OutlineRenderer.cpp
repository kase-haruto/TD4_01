#include "OutlineRenderer.h"

#include <Engine\Assets\Animation\AnimationModel.h>
#include <Engine\Assets\Model\BaseModel.h>
#include <Engine\Graphics\Camera\3d\Camera3d.h>
#include <Engine\Graphics\Pipeline\Pso\PsoDetails.h>
#include <Engine\Graphics\RenderTarget\Interface\IRenderTarget.h>
#include <Engine\Objects\3D\Actor\SceneObject.h>

#include <algorithm>

bool OutlineRenderer::SameConstants(const Constants& lhs, const Constants& rhs) {
	return lhs.color.x == rhs.color.x &&
		   lhs.color.y == rhs.color.y &&
		   lhs.color.z == rhs.color.z &&
		   lhs.color.w == rhs.color.w &&
		   lhs.thickness == rhs.thickness;
}

OutlineRenderer::Constants OutlineRenderer::MakeConstants(const SceneObject& owner) {
	const auto& settings = owner.GetOutlineSettings();
	return Constants{
		settings.color,
		settings.thickness > 0.0f ? settings.thickness : 0.0f};
}

void OutlineRenderer::Render(ID3D12GraphicsCommandList* cmdList,
							 ID3D12Device*				device,
							 IRenderTarget*				rt,
							 PipelineService*			psoService,
							 const Camera3d*			camera,
							 const ModelRenderer&		modelRenderer) {
	if(!cmdList || !device || !rt || !psoService || !camera) return;

	std::vector<ModelRenderer::RenderInstance> staticInstances;
	std::vector<ModelRenderer::RenderInstance> skinnedInstances;
	modelRenderer.CollectVisibleStatic(staticInstances);
	modelRenderer.CollectVisibleSkinned(skinnedInstances);

	std::vector<StaticBatch> staticBatches;
	for(const auto& inst : staticInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(!inst.owner->IsOutlineEnabled()) continue;
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;

		const Constants constants = MakeConstants(*inst.owner);
		if(constants.thickness <= 0.0f || constants.color.w <= 0.0f) continue;

		auto it = std::find_if(staticBatches.begin(), staticBatches.end(),
							   [&](const StaticBatch& batch) {
								   return batch.model == inst.model &&
										  SameConstants(batch.constants, constants);
							   });
		if(it == staticBatches.end()) {
			StaticBatch batch;
			batch.model		= inst.model;
			batch.constants = constants;
			staticBatches.emplace_back(std::move(batch));
			it = std::prev(staticBatches.end());
		}

		it->transforms.push_back(*inst.transform);
		GpuBillboardParams billboard{};
		billboard.mode = static_cast<uint32_t>(inst.billboardMode);
		it->billboards.push_back(billboard);
	}

	if(!staticBatches.empty()) {
		rt->SetRenderTarget(cmdList);
		const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineObject3D, BlendMode::NONE);
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

			cmdList->SetGraphicsRoot32BitConstants(3, 5, &batch.constants, 0);
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

		const Constants constants = MakeConstants(*inst.owner);
		if(constants.thickness <= 0.0f || constants.color.w <= 0.0f) continue;

		auto* model = static_cast<CalyxEngine::AnimationModel*>(inst.model);
		if(!skinnedPipelineSet) {
			rt->SetRenderTarget(cmdList);
			const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineSkinnedObject3D, BlendMode::NONE);
			psoService->SetCommand(ps, cmdList);
			camera->SetRootCommand(cmdList, 4);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			skinnedPipelineSet = true;
		}

		cmdList->SetGraphicsRoot32BitConstants(3, 5, &constants, 0);
		model->Draw(*inst.transform);
	}
}
