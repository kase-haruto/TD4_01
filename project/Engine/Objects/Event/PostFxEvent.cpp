#include "PostFxEvent.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetRecord.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>

#include <externals/imgui/imgui.h>

REGISTER_SCENE_OBJECT(PostFxEvent)

PostFxEvent::PostFxEvent()
	: BaseEventObject("PostFxEvent") {}

PostFxEvent::PostFxEvent(const std::string& name)
	: BaseEventObject(name) {}

void PostFxEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);

	if(!enabled_ || !postEffectGuid_.isValid()) return;
	PostEffectManager::Get()->RequestScenePreset(postEffectGuid_, priority_);
}

void PostFxEvent::DerivativeGui() {
	ImGui::SeparatorText("Post Effect");
	ImGui::Checkbox("Enabled", &enabled_);
	ImGui::DragInt("Priority", &priority_, 1.0f);

	if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::PostEffect, &postEffectGuid_)) {
		PostEffectManager::Get()->LoadPresetFromAsset(postEffectGuid_);
	}

	const AssetRecord* record = postEffectGuid_.isValid()
		? AssetDatabase::GetInstance()->Get(postEffectGuid_)
		: nullptr;
	ImGui::TextDisabled("Preset: %s",
						(record && record->type == AssetType::PostEffect)
							? record->sourcePath.filename().string().c_str()
							: "(none)");

	if(ImGui::Button("Clear Preset")) {
		postEffectGuid_ = Guid::Empty();
	}
}

void PostFxEvent::ApplyDerivedConfigFromJson([[maybe_unused]] const nlohmann::json& root,
											 const nlohmann::json* derived) {
	if(!derived || !derived->is_object()) return;
	postEffectGuid_ = derived->value("postEffectGuid", Guid::Empty());
	enabled_ = derived->value("enabled", enabled_);
	priority_ = derived->value("priority", priority_);
}

void PostFxEvent::ExtractDerivedConfigToJson([[maybe_unused]] nlohmann::json& root,
											 nlohmann::json& derived) const {
	derived["postEffectGuid"] = postEffectGuid_;
	derived["enabled"] = enabled_;
	derived["priority"] = priority_;
}
