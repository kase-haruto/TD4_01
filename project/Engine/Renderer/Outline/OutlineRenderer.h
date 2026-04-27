#pragma once

#include <Engine\Foundation\Math\Vector4.h>
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
	struct Constants {
		CalyxEngine::Vector4 color;
		float				 thickness = 0.0f;
	};

	struct StaticBatch {
		BaseModel*						   model = nullptr;
		Constants						   constants{};
		std::vector<WorldTransform>		   transforms;
		std::vector<GpuBillboardParams>	   billboards;
	};

	static bool SameConstants(const Constants& lhs, const Constants& rhs);
	static Constants MakeConstants(const SceneObject& owner);
};
