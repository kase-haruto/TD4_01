#include "TutorialEvent.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <externals/imgui/imgui.h>

#include <array>
#include <algorithm>

REGISTER_SCENE_OBJECT(TutorialEvent)

namespace {
	// 画面中央に表示する想定の初期レイアウト
	constexpr float kTutorialPosX	= 640.0f;
	constexpr float kTutorialPosY	= 360.0f;
	constexpr float kTutorialScaleX = 640.0f;
	constexpr float kTutorialScaleY = 360.0f;
} // namespace

TutorialEvent::TutorialEvent(const std::string& name) : StageGimmickEventBase(name) {}

void TutorialEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// 一度きり。発火済み/完了済みなら何もしない
	if(state_ != TutorialState::Idle) return;

	// 時間を止めてチュートリアルを表示
	ClockManager::GetInstance()->SetTimeScale(0.0f);

	state_			 = TutorialState::Active;
	divePhase_		 = DivePhase::WaitFirstInput;
	diveResumeTimer_ = 0.0f;
	// 連打対策：表示直後しばらくは入力を受け付けない
	inputBlockTimer_ = eventData_.inputBlockDuration;

	ApplyTextureForType();
	if(sprite_) {
		sprite_->SetVisibility(true);
	}
}

void TutorialEvent::EventInitialize() {

	const std::string eventPrefix = "TutorialEvent";
	if(GetName() != eventPrefix) {
		return;
	}
}

void TutorialEvent::EventUpdate(float dt) {
	if(!hasSerializedEventData_) {
		eventData_.SetEventName(GetName());
		eventData_.LoadParams();
		EventInitialize();
	}
	if(state_ == TutorialState::Idle) lastDt_ = dt;

	// スプライトの更新（生成済みのときのみ）
	// dt は時間停止（TimeScale=0）の影響を受けないグローバルdtなので、
	// 表示中でも各種タイマーは正しく進む
	if(sprite_) {
		sprite_->SetPosition({kTutorialPosX, kTutorialPosY});
		sprite_->Update(dt);
	}

	if(state_ != TutorialState::Active) {
		return;
	}

	// 連打対策：一定時間は入力を反映しない
	if(inputBlockTimer_ > 0.0f) {
		inputBlockTimer_ -= dt;
		return;
	}

	switch(static_cast<TutorialType>(eventData_.type)) {
	case Dive:
		UpdateDive(dt);
		break;
	case Jump:
	case Purpose:
	default:
		// Aボタン または スペースで解除
		if(IsConfirmTriggered()) {
			Finish();
		}
		break;
	}
}

void TutorialEvent::UpdateDive(float dt) {
	switch(divePhase_) {
	case DivePhase::WaitFirstInput:
		// 1回目の入力で一度だけ時間を進める
		if(IsConfirmTriggered()) {
			ClockManager::GetInstance()->SetTimeScale(1.0f);
			diveResumeTimer_ = eventData_.diveResumeDuration;
			divePhase_		 = DivePhase::Playing;
		}
		break;

	case DivePhase::Playing:
		// 指定時間だけ進めたら再び時間を止める
		diveResumeTimer_ -= dt;
		if(diveResumeTimer_ <= 0.0f) {
			ClockManager::GetInstance()->SetTimeScale(0.0f);
			divePhase_ = DivePhase::WaitSecondInput;
			// 再度の連打対策
			inputBlockTimer_ = eventData_.inputBlockDuration;
		}
		break;

	case DivePhase::WaitSecondInput:
		// 2回目の入力で解除
		if(IsConfirmTriggered()) {
			Finish();
		}
		break;
	}
}

void TutorialEvent::Finish() {
	ClockManager::GetInstance()->SetTimeScale(1.0f);
	state_ = TutorialState::Finished;
	if(sprite_) {
		sprite_->SetVisibility(false);
	}
}

bool TutorialEvent::IsConfirmTriggered() {
	using CalyxFoundation::Input;
	return Input::TriggerGamepadButton(CalyxFoundation::PadButton::A) ||
		   Input::TriggerKey(DIK_SPACE);
}

void TutorialEvent::EnsureSprite() {
	if(sprite_) return;
	sprite_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	sprite_->Initialize(jumpTexturePath_);
	sprite_->SetAnchorPoint({0.5f, 0.5f});
	sprite_->SetPosition({kTutorialPosX, kTutorialPosY});
	sprite_->SetScale({kTutorialScaleX, kTutorialScaleY});
	sprite_->SetVisibility(false);
}

void TutorialEvent::ApplyTextureForType() {
	EnsureSprite();
	switch(static_cast<TutorialType>(eventData_.type)) {
	case Jump:
		sprite_->SetTexture(jumpTexturePath_);
		break;
	case Dive:
		sprite_->SetTexture(diveTexturePath_);
		break;
	case Purpose:
		sprite_->SetTexture(purposeTexturePath_);
		break;
	}
}

void TutorialEvent::DrawSprite(SpriteRenderer* renderer) const {
	if(sprite_ && renderer && state_ == TutorialState::Active) {
		sprite_->Draw(renderer);
	}
}

void TutorialEvent::DerivativeGui() {
	eventData_.ShowGui();

	ImGui::SeparatorText("Tutorial Type");
	const char* typeNames[] = {"Jump", "Dive", "Purpose"};
	int			currentType = std::clamp(eventData_.type, 0, 2);
	if(ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
		eventData_.type = currentType;
		// 表示中なら即座に反映
		if(sprite_) {
			ApplyTextureForType();
		}
	}

	ImGui::SeparatorText("Textures");
	auto editTexturePath = [](const char* label, std::string& path) {
		std::array<char, 256> buffer{};
		std::copy_n(path.c_str(), (std::min)(path.size(), buffer.size() - 1), buffer.data());
		if(ImGui::InputText(label, buffer.data(), buffer.size())) {
			path = buffer.data();
		}
	};
	editTexturePath("Jump Texture", jumpTexturePath_);
	editTexturePath("Dive Texture", diveTexturePath_);
	editTexturePath("Purpose Texture", purposeTexturePath_);
}

void TutorialEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventData")) {
		eventData_.ApplyParamsFromJson(derived->at("eventData"));
		hasSerializedEventData_ = true;
	}
	if(derived->contains("jumpTexture")) {
		jumpTexturePath_ = derived->at("jumpTexture").get<std::string>();
	}
	if(derived->contains("diveTexture")) {
		diveTexturePath_ = derived->at("diveTexture").get<std::string>();
	}
	if(derived->contains("purposeTexture")) {
		purposeTexturePath_ = derived->at("purposeTexture").get<std::string>();
	}
}

void TutorialEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventData_.ExtractParamsToJson(derived["eventData"]);
	derived["jumpTexture"]	  = jumpTexturePath_;
	derived["diveTexture"]	  = diveTexturePath_;
	derived["purposeTexture"] = purposeTexturePath_;
}

void TutorialEvent::RemapSceneObjectReferences([[maybe_unused]] const std::unordered_map<Guid, Guid>& guidMap) {
}
