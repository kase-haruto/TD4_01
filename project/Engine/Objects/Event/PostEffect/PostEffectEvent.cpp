#include "PostEffectEvent.h"

#include <Engine\Application\UI\Panels\AssetPanel.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\System\AssetRecord.h>
#include <Engine\Objects\3D\Actor\Registry\SceneObjectRegistry.h>
#include <Engine\PostProcess\Manager\PostEffectManager.h>

#include <externals\imgui\imgui.h>

REGISTER_SCENE_OBJECT(PostEffectEvent)

PostEffectEvent::PostEffectEvent() = default;

PostEffectEvent::PostEffectEvent(const std::string& name)
	: BaseEventObject(name) {}

void PostEffectEvent::Initialize() {
	BaseEventObject::Initialize();
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		model_->SetColor(CalyxEngine::Vector4(0.2f, 0.45f, 1.0f, 0.5f));
	}
	ApplyPresetIfNeeded(true);
}

void PostEffectEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);
	ApplyPresetIfNeeded(false);
}

void PostEffectEvent::DerivativeGui() {
	ImGui::Checkbox("Enabled", &enabled_);
	if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::PostEffect, &postEffectGuid_)) {
		loaded_ = false;
		ApplyPresetIfNeeded(true);
	}
	ImGui::TextDisabled("Current: %s", PostEffectLabel().c_str());
	if(postEffectGuid_.isValid()) {
		ImGui::SameLine();
		if(ImGui::SmallButton("Clear")) {
			postEffectGuid_ = Guid{};
			loaded_ = false;
			PostEffectManager::Get()->ClearScenePostEffect();
		}
	}
	if(ImGui::Button("Apply")) {
		ApplyPresetIfNeeded(true);
	}
}

void PostEffectEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	postEffectGuid_ = derived->value("postEffectGuid", Guid{});
	if(!postEffectGuid_.isValid()) {
		const std::string legacyPath = derived->value("presetPath", "");
		if(!legacyPath.empty()) {
			if(auto* db = AssetDatabase::GetInstance()) {
				if(const AssetRecord* record = db->FindByPath(legacyPath); record && record->type == AssetType::PostEffect) {
					postEffectGuid_ = record->guid;
				}
			}
		}
	}
	enabled_ = derived->value("enabled", enabled_);
	loaded_ = false;
}

void PostEffectEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	derived["postEffectGuid"] = postEffectGuid_;
	derived["enabled"] = enabled_;
}

void PostEffectEvent::ApplyPresetIfNeeded(bool force) {
	auto* manager = PostEffectManager::Get();
	if(!manager || !manager->IsInitialized()) return;

	if(!enabled_ || !postEffectGuid_.isValid()) {
		manager->ClearScenePostEffect();
		loaded_ = false;
		loadedGuid_ = Guid{};
		return;
	}

	if(!force && loaded_ && loadedGuid_ == postEffectGuid_) {
		return;
	}

	const AssetRecord* record = nullptr;
	if(auto* db = AssetDatabase::GetInstance()) {
		record = db->Get(postEffectGuid_);
	}

	if(record && record->type == AssetType::PostEffect && manager->LoadPreset(record->sourcePath.string())) {
		loaded_ = true;
		loadedGuid_ = postEffectGuid_;
	} else {
		manager->ClearScenePostEffect();
		loaded_ = false;
		loadedGuid_ = Guid{};
	}
}

std::string PostEffectEvent::PostEffectLabel() const {
	if(!postEffectGuid_.isValid()) return "(none)";
	if(auto* db = AssetDatabase::GetInstance()) {
		if(const AssetRecord* record = db->Get(postEffectGuid_); record && record->type == AssetType::PostEffect) {
			return record->sourcePath.filename().string();
		}
	}
	return postEffectGuid_.ToString();
}
