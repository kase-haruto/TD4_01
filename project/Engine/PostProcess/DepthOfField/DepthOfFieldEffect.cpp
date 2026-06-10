#include "DepthOfFieldEffect.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void DepthOfFieldEffect::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());

	ResetParameters();
}

void DepthOfFieldEffect::Apply(ID3D12GraphicsCommandList* cmd,
							   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
							   IRenderTarget* outputRT) {
	// 深度SRVが無い呼び出しでは、効果をかけずに入力色だけをコピーする。
	if(auto* copy = PostEffectManager::Get()->GetPass("CopyImage")) {
		copy->Apply(cmd, inputSRV, outputRT);
	}
}

void DepthOfFieldEffect::Apply(ID3D12GraphicsCommandList* cmd,
							   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
							   D3D12_GPU_DESCRIPTOR_HANDLE depthSRV,
							   IRenderTarget* outputRT) {
	// 深度が未設定の場合は安全に素通しする。
	if(depthSRV.ptr == 0) {
		Apply(cmd, inputSRV, outputRT);
		return;
	}

	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	outputRT->SetRenderTarget(cmd);

	// 現在のパラメータをGPUへ転送してから、カラーと深度を同時に参照する。
	buffer_.TransferData(param_);
	psoSet_.SetCommand(cmd);

	cmd->SetGraphicsRootDescriptorTable(0, inputSRV); // t0 : scene color
	cmd->SetGraphicsRootDescriptorTable(1, depthSRV); // t1 : scene depth
	buffer_.SetCommand(cmd, 2);                       // b0 : DepthOfFieldParameter

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void DepthOfFieldEffect::ShowImGui() {
	if(ImGui::CollapsingHeader("DepthOfField")) {
		ImGui::SliderFloat("Focus Depth", &param_.focusDepth, 0.0f, 1.0f);
		ImGui::SliderFloat("Focus Range", &param_.focusRange, 0.001f, 0.25f);
		ImGui::SliderFloat("Max Blur Radius", &param_.maxBlurRadius, 0.0f, 24.0f);
		ImGui::SliderFloat("Intensity", &param_.intensity, 0.0f, 2.0f);
		ImGui::SliderFloat("Foreground Blur", &param_.foregroundBlur, 0.0f, 1.0f);
		ImGui::DragFloat("Near Plane", &param_.nearPlane, 0.01f, 0.001f, 100.0f);
		ImGui::DragFloat("Far Plane", &param_.farPlane, 1.0f, 1.0f, 10000.0f);
		if(ImGui::Button("Reset")) {
			ResetParameters();
		}
	}
}

void DepthOfFieldEffect::ResetParameters() {
	param_.focusDepth = 0.03f;
	param_.focusRange = 0.02f;
	param_.maxBlurRadius = 6.0f;
	param_.intensity = 1.0f;
	param_.nearPlane = 0.1f;
	param_.farPlane = 100.0f;
	param_.foregroundBlur = 0.0f;
	param_.padding = 0.0f;
}

nlohmann::json DepthOfFieldEffect::SaveParameters() const {
	return nlohmann::json{
		{"focusDepth", param_.focusDepth},
		{"focusRange", param_.focusRange},
		{"maxBlurRadius", param_.maxBlurRadius},
		{"intensity", param_.intensity},
		{"nearPlane", param_.nearPlane},
		{"farPlane", param_.farPlane},
		{"foregroundBlur", param_.foregroundBlur}
	};
}

void DepthOfFieldEffect::LoadParameters(const nlohmann::json& params) {
	if(params.contains("focusDepth") && params["focusDepth"].is_number()) {
		SetFocusDepth(params["focusDepth"].get<float>());
	}
	if(params.contains("focusRange") && params["focusRange"].is_number()) {
		SetFocusRange(params["focusRange"].get<float>());
	}
	if(params.contains("maxBlurRadius") && params["maxBlurRadius"].is_number()) {
		SetMaxBlurRadius(params["maxBlurRadius"].get<float>());
	}
	if(params.contains("intensity") && params["intensity"].is_number()) {
		SetIntensity(params["intensity"].get<float>());
	}
	if(params.contains("nearPlane") && params["nearPlane"].is_number()) {
		SetNearPlane(params["nearPlane"].get<float>());
	}
	if(params.contains("farPlane") && params["farPlane"].is_number()) {
		SetFarPlane(params["farPlane"].get<float>());
	}
	if(params.contains("foregroundBlur") && params["foregroundBlur"].is_number()) {
		SetForegroundBlur(params["foregroundBlur"].get<float>());
	}
}

bool DepthOfFieldEffect::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "focusDepth") {
		out = param_.focusDepth;
		return true;
	}
	if(name == "focusRange") {
		out = param_.focusRange;
		return true;
	}
	if(name == "maxBlurRadius") {
		out = param_.maxBlurRadius;
		return true;
	}
	if(name == "intensity") {
		out = param_.intensity;
		return true;
	}
	if(name == "nearPlane") {
		out = param_.nearPlane;
		return true;
	}
	if(name == "farPlane") {
		out = param_.farPlane;
		return true;
	}
	if(name == "foregroundBlur") {
		out = param_.foregroundBlur;
		return true;
	}
	return false;
}

bool DepthOfFieldEffect::SetFloatParameter(const std::string& name, float value) {
	if(name == "focusDepth") {
		SetFocusDepth(value);
		return true;
	}
	if(name == "focusRange") {
		SetFocusRange(value);
		return true;
	}
	if(name == "maxBlurRadius") {
		SetMaxBlurRadius(value);
		return true;
	}
	if(name == "intensity") {
		SetIntensity(value);
		return true;
	}
	if(name == "nearPlane") {
		SetNearPlane(value);
		return true;
	}
	if(name == "farPlane") {
		SetFarPlane(value);
		return true;
	}
	if(name == "foregroundBlur") {
		SetForegroundBlur(value);
		return true;
	}
	return false;
}

void DepthOfFieldEffect::SetFocusDepth(float value) {
	param_.focusDepth = std::clamp(value, 0.0f, 1.0f);
}

void DepthOfFieldEffect::SetFocusRange(float value) {
	param_.focusRange = std::clamp(value, 0.001f, 1.0f);
}

void DepthOfFieldEffect::SetMaxBlurRadius(float value) {
	param_.maxBlurRadius = std::clamp(value, 0.0f, 32.0f);
}

void DepthOfFieldEffect::SetIntensity(float value) {
	param_.intensity = std::clamp(value, 0.0f, 4.0f);
}

void DepthOfFieldEffect::SetNearPlane(float value) {
	param_.nearPlane = std::clamp(value, 0.001f, 1000.0f);
	if(param_.farPlane <= param_.nearPlane) {
		param_.farPlane = param_.nearPlane + 0.001f;
	}
}

void DepthOfFieldEffect::SetFarPlane(float value) {
	param_.farPlane = std::clamp(value, 0.002f, 10000.0f);
	if(param_.farPlane <= param_.nearPlane) {
		param_.nearPlane = (std::max)(0.001f, param_.farPlane - 0.001f);
	}
}

void DepthOfFieldEffect::SetForegroundBlur(float value) {
	param_.foregroundBlur = std::clamp(value, 0.0f, 1.0f);
}
