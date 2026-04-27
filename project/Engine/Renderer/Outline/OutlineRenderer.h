#pragma once

#include <Engine\Foundation\Math\Vector2.h>
#include <Engine\Foundation\Math\Vector4.h>
#include <Engine\Graphics\Descriptor\DescriptorAllocator.h>
#include <Engine\Graphics\GpuResource\DxGpuResource.h>
#include <Engine\Renderer\Model\ModelRenderer.h>

#include <d3d12.h>
#include <vector>

class Camera3d;
class IRenderTarget;
class PipelineService;

class OutlineRenderer {
public:
	void Render(ID3D12GraphicsCommandList* cmdList,
				ID3D12Device* device,
				IRenderTarget* rt,
				PipelineService* psoService,
				const Camera3d* camera,
				const ModelRenderer& modelRenderer);

private:
	struct CompositeConstants {
		CalyxEngine::Vector4 color = {0.02f, 0.02f, 0.025f, 1.0f};
		CalyxEngine::Vector2 texelSize = {1.0f, 1.0f};
		float depthThreshold = 0.0025f;
		float normalThreshold = 0.18f;
		float depthScale = 90.0f;
		float normalScale = 1.8f;
		float thickness = 2.0f;
		float padding = 0.0f;
	};

	struct StaticBatch {
		BaseModel*						   model = nullptr;
		std::vector<WorldTransform>		   transforms;
		std::vector<GpuBillboardParams>	   billboards;
	};

	void EnsureResources(ID3D12Device* device, const D3D12_VIEWPORT& viewport);
	void RenderNormalBuffer(ID3D12GraphicsCommandList* cmdList,
							ID3D12Device* device,
							IRenderTarget* rt,
							PipelineService* psoService,
							const Camera3d* camera,
							const ModelRenderer& modelRenderer);
	void Composite(ID3D12GraphicsCommandList* cmdList,
				   IRenderTarget* rt,
				   PipelineService* psoService);

	DxGpuResource normalResource_;
	DxGpuResource compositeResource_;
	DescriptorHandle normalRtv_{};
	DescriptorHandle compositeRtv_{};
	CompositeConstants compositeConstants_{};
	uint32_t width_ = 0;
	uint32_t height_ = 0;
};
