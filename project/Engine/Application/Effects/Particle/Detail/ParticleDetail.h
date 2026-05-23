
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

namespace CalyxEngine {
	/// <summary>
	/// particleCBデータ
	/// </summary>
	struct ParticleConstantData {
		CalyxEngine::Vector3 position;
		CalyxEngine::Vector3 scale;
		CalyxEngine::Vector4 color;
		CalyxEngine::Vector3 rotation{0.0f, 0.0f, 0.0f};
	};
}
