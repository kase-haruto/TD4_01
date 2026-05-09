#pragma once

#include <Engine\Foundation\Math\Vector4.h>
#include <Engine\Graphics\Buffer\DxConstantBuffer.h>

#include <d3d12.h>

class BaseCamera;
class PipelineService;

namespace CalyxEngine {

	class GridRenderer {
	public:
		struct Settings {
			float minorSpacing = 0.1f;
			float majorSpacing = 1.0f;
			float axisWidth	= 0.018f;
			float minorAlpha = 0.25f;
			float majorAlpha = 0.45f;
			float fadeStart	= 40.0f;
			float fadeEnd = 180.0f;
			float horizonFade = 0.08f;
			Vector4 minorColor{0.36f, 0.38f, 0.42f, 0.35f};
			Vector4 majorColor{0.58f, 0.60f, 0.66f, 0.55f};
			Vector4 xAxisColor{1.00f, 0.18f, 0.16f, 0.95f};
			Vector4 yAxisColor{0.18f, 0.32f, 1.00f, 0.85f};
			Vector4 zAxisColor{0.20f, 0.90f, 0.30f, 0.95f};
		};

		void Initialize();
		void Render(ID3D12GraphicsCommandList* cmd, PipelineService* pso, BaseCamera* camera);

		Settings& GetSettings() { return settings_; }
		const Settings& GetSettings() const { return settings_; }

	private:
		Settings settings_{};
		DxConstantBuffer<Settings> settingsBuffer_;
		bool initialized_ = false;
	};

} // namespace CalyxEngine
