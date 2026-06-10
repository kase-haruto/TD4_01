#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

/*-----------------------------------------------------------------------------------------
 * DepthOfFieldEffect
 * - 被写界深度を適用するポストエフェクト
 * - シーンカラーと深度テクスチャを参照し、焦点深度から離れた画素をぼかす
 *---------------------------------------------------------------------------------------*/
class DepthOfFieldEffect : public IPostEffectPass {
private:
	/*-----------------------------------------------------------------------------------------
	 * DepthOfFieldParameter
	 * - HLSLへ送る被写界深度パラメータ
	 * - 16byte境界を保つため、float4相当の構成にしている
	 *---------------------------------------------------------------------------------------*/
	struct DepthOfFieldParameter {
		float focusDepth = 0.03f;    // ピントが合うリニア深度。0.0が近く、1.0が遠い
		float focusRange = 0.02f;    // ピントが合って見えるリニア深度幅
		float maxBlurRadius = 6.0f;  // 最大ぼかし半径。ピクセル単位
		float intensity = 1.0f;      // ぼかしのかかり具合
		float nearPlane = 0.1f;      // 深度をリニア化するときのNear平面
		float farPlane = 100.0f;     // 深度をリニア化するときのFar平面
		float foregroundBlur = 0.0f; // 焦点より手前をぼかす強さ。背景ぼかし用途では0にする
		float padding = 0.0f;
	};

public:
	/**
	 * \brief 初期化
	 * \param psoSet 被写界深度用パイプラインセット
	 */
	void Initialize(const PipelineSet& psoSet);

	/**
	 * \brief 深度なし経路から呼ばれた場合の描画
	 * \param cmd コマンドリスト
	 * \param inputSRV 入力カラーSRV
	 * \param outputRT 出力先RT
	 */
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	/**
	 * \brief 被写界深度を適用
	 * \param cmd コマンドリスト
	 * \param inputSRV 入力カラーSRV
	 * \param depthSRV シーン深度SRV
	 * \param outputRT 出力先RT
	 */
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   D3D12_GPU_DESCRIPTOR_HANDLE depthSRV,
			   IRenderTarget* outputRT) override;

	const std::string GetName() const override { return "DepthOfField"; }

	void ShowImGui() override;
	void ResetParameters() override;
	nlohmann::json SaveParameters() const override;
	void LoadParameters(const nlohmann::json& params) override;
	bool GetFloatParameter(const std::string& name, float& out) const override;
	bool SetFloatParameter(const std::string& name, float value) override;

private:
	void SetFocusDepth(float value);
	void SetFocusRange(float value);
	void SetMaxBlurRadius(float value);
	void SetIntensity(float value);
	void SetNearPlane(float value);
	void SetFarPlane(float value);
	void SetForegroundBlur(float value);

private:
	PipelineSet psoSet_{};
	DepthOfFieldParameter param_{};
	DxConstantBuffer<DepthOfFieldParameter> buffer_;
};
