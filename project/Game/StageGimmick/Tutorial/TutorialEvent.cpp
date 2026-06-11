#include "TutorialEvent.h"

#include <Game/DemoPlayer/DemoPlayer.h>

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <externals/imgui/imgui.h>

#include <array>
#include <algorithm>

REGISTER_SCENE_OBJECT(TutorialEvent)

TutorialEvent::TutorialEvent(const std::string& name) : StageGimmickEventBase(name) {}

void TutorialEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// 一度きり。発火済み/完了済みなら何もしない
	if(state_ != TutorialState::Idle) return;

	// プレイヤーに自分を登録する。連打対策中は
	// プレイヤー側でジャンプ入力が無視される（Diveの2回目停止にも対応）ジャンプ中に入ったら知らん
	if(auto* player = dynamic_cast<DemoPlayer*>(other->GetOwner())) {
		player->SetActiveTutorial(this);
	}

	// 時間を止めてチュートリアルを表示
	ClockManager::GetInstance()->SetTimeScale(0.0f);

	state_			 = TutorialState::Active;
	divePhase_		 = DivePhase::WaitFirstInput;
	diveResumeTimer_ = 0.0f;
	// 連打対策：表示直後しばらくは入力を受け付けない
	inputBlockTimer_ = eventData_.inputBlockDuration;

	ApplyTextureForType();

	// 中央から拡大して出すアニメーションを開始
	StartPopIn();

	if(spritePad_) {
		spritePad_->SetVisibility(true);
	}
	if(sprite_) {
		sprite_->SetVisibility(true);
	}
}

void TutorialEvent::EventInitialize() {

	// 初期化時にパッドが接続されていればパッド表示、なければキーボード表示
	isPad_ = CalyxFoundation::Input::IsGamepadConnected();

	worldTransform_.scale.x = 20.0f;
	worldTransform_.scale.y = 10.0f;

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

	// ポップイン/アウトの進行
	if(popInTimer_ > 0.0f) {
		popInTimer_ -= dt;
		if(popInTimer_ < 0.0f) popInTimer_ = 0.0f;
	}
	if(state_ == TutorialState::Closing && popOutTimer_ > 0.0f) {
		popOutTimer_ -= dt;
		if(popOutTimer_ <= 0.0f) {
			// 縮小しきったので非表示にして完了
			popOutTimer_ = 0.0f;
			state_		 = TutorialState::Finished;
			if(sprite_) sprite_->SetVisibility(false);
			if(spritePad_) spritePad_->SetVisibility(false);
		}
	}
	const float popScale = CurrentPopScale();
	const CalyxEngine::Vector2 animScale = {spriteScale_.x * popScale, spriteScale_.y * popScale};

	if(spritePad_) {
		spritePad_->SetPosition(spritePosition_);
		spritePad_->SetScale(animScale);
		spritePad_->Update(dt);
	}
	if(sprite_) {
		sprite_->SetPosition(spritePosition_);
		sprite_->SetScale(animScale);
		sprite_->Update(dt);
	}

	if(state_ != TutorialState::Active) {
		return;
	}

	// 入力デバイスの切り替え判定
	UpdateInputDevice();

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
	case Teeth:
	case Open:
	case Choco:
	default:
		// Aボタン または スペースで解除（一回停止）
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
	// 時間は通常に戻す、スプライトはまだ消さず縮小アニメーションへ移行する
	ClockManager::GetInstance()->SetTimeScale(1.0f);
	state_ = TutorialState::Closing;
	StartPopOut();
}

bool TutorialEvent::IsConfirmTriggered() {
	using CalyxFoundation::Input;
	return Input::TriggerGamepadButton(CalyxFoundation::PadButton::A) ||
		   Input::TriggerKey(DIK_SPACE);
}

void TutorialEvent::UpdateInputDevice() {
	using CalyxFoundation::Input;
	using CalyxFoundation::PadButton;

	// チェック対象のパッドボタン一覧（PadButtonはXInputのビットフラグなので明示列挙）
	static constexpr PadButton kButtons[] = {
		PadButton::A,
		PadButton::B,
		PadButton::X,
		PadButton::Y,
		PadButton::LB,
		PadButton::RB,
		PadButton::BACK,
		PadButton::START,
		PadButton::L_STICK,
		PadButton::R_STICK,
		PadButton::DPAD_UP,
		PadButton::DPAD_DOWN,
		PadButton::DPAD_LEFT,
		PadButton::DPAD_RIGHT,
	};

	// いずれかのボタンに触れたらパッド表示に切り替え
	for(PadButton button : kButtons) {
		if(Input::PushGamepadButton(button)) {
			isPad_ = true;
			if(spritePad_) {
				spritePad_->SetVisibility(true);
			}
			if(sprite_) {
				sprite_->SetVisibility(false);
			}
			return;
		}
	}

	 // トリガー・スティックも「パッドに触った」として扱う
	constexpr float kTriggerThreshold = 0.1f;
	if(Input::GetLeftTrigger() > kTriggerThreshold ||
	   Input::GetRightTrigger() > kTriggerThreshold ||
	   Input::IsLeftStickMoved()) {
		isPad_ = true;
		if(spritePad_) {
			spritePad_->SetVisibility(true);
		}
		if(sprite_) {
			sprite_->SetVisibility(false);
		}
		return;
	}

	// キーボードに触れたらキーボード表示に戻す（任意：不要なら削除）
	for(uint32_t key = 0; key < 256; ++key) {
		if(Input::PushKey(key)) {
			isPad_ = false;
			if(spritePad_) {
				spritePad_->SetVisibility(false);
			}
			if(sprite_) {
				sprite_->SetVisibility(true);
			}
			return;
		}
	}
}

void TutorialEvent::StartPopIn() {
	popInTimer_ = popInDuration_;
}

void TutorialEvent::StartPopOut() {
	popOutTimer_ = popOutDuration_;
}

float TutorialEvent::EaseOutBack(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	constexpr float c1 = 1.70158f;
	constexpr float c3 = c1 + 1.0f;
	const float p = t - 1.0f;
	return 1.0f + c3 * p * p * p + c1 * p * p;
}

float TutorialEvent::CurrentPopScale() const {
	if(state_ == TutorialState::Closing) {
		if(popOutDuration_ <= 0.0f) return 0.0f;
		// 残り時間の割合 : 1(開始) -> 0(完了)
		const float remain = std::clamp(popOutTimer_ / popOutDuration_, 0.0f, 1.0f);
		return EaseOutBack(remain);
	}

	// 拡大中：0 -> 1
	if(popInDuration_ <= 0.0f || popInTimer_ <= 0.0f) {
		return 1.0f;
	}
	const float t = 1.0f - (popInTimer_ / popInDuration_);
	return EaseOutBack(t);
}

void TutorialEvent::EnsureSprite() {
	if(!sprite_) {
		sprite_ = std::make_unique<CalyxEngine::SpriteObject2d>();
		sprite_->Initialize(jumpTexturePath_);
		sprite_->SetAnchorPoint({0.5f, 0.5f});
		sprite_->SetPosition(spritePosition_);
		sprite_->SetScale(spriteScale_);
		sprite_->SetVisibility(false);
	}
	if(!spritePad_) {
		spritePad_ = std::make_unique<CalyxEngine::SpriteObject2d>();
		spritePad_->Initialize(jumpTexturePath_);
		spritePad_->SetAnchorPoint({0.5f, 0.5f});
		spritePad_->SetPosition(spritePosition_);
		spritePad_->SetScale(spriteScale_);
		spritePad_->SetVisibility(false);
	}
}

void TutorialEvent::ApplyTextureForType() {
	EnsureSprite();
	switch(static_cast<TutorialType>(eventData_.type)) {
	case Jump:
		sprite_->SetTexture(jumpTexturePath_);
		spritePad_->SetTexture(jumpTexturePadPath_);
		break;
	case Dive:
		sprite_->SetTexture(diveTexturePath_);
		spritePad_->SetTexture(diveTexturePadPath_);
		break;
	case Purpose:
		sprite_->SetTexture(purposeTexturePath_);
		spritePad_->SetTexture(purposeTexturePadPath_);
		break;
	case Teeth:
		sprite_->SetTexture(teethTexturePath_);
		spritePad_->SetTexture(teethTexturePadPath_);
		break;
	case Open:
		sprite_->SetTexture(openTexturePath_);
		spritePad_->SetTexture(openTexturePadPath_);
		break;
	case Choco:
		sprite_->SetTexture(chocoTexturePath_);
		spritePad_->SetTexture(chocoTexturePadPath_);
		break;
	}
}

void TutorialEvent::DrawSprite(SpriteRenderer* renderer) const {
	// Active、Closing中描画する
	const bool visible = (state_ == TutorialState::Active || state_ == TutorialState::Closing);
	if(!renderer || !visible) {
		return;
	}
	if(isPad_) {
		if(spritePad_) {
			spritePad_->Draw(renderer);
		}
		if(sprite_) {
			sprite_->Draw(renderer);
		}
	} else {
		if(sprite_) {
			sprite_->Draw(renderer);
		}
		if(spritePad_) {
			spritePad_->Draw(renderer);
		}
	}
}

void TutorialEvent::DerivativeGui() {
	eventData_.ShowGui();

	ImGui::SeparatorText("Tutorial Type");
	const char* typeNames[] = {"Jump", "Dive", "Purpose", "Teeth", "Open", "Choco"};
	int			currentType = std::clamp(eventData_.type, 0, static_cast<int>(IM_ARRAYSIZE(typeNames) - 1));
	if(ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
		eventData_.type = currentType;
		// 表示中なら即座に反映
		if(sprite_) {
			ApplyTextureForType();
		}
	}

	ImGui::SeparatorText("Sprite Layout");
	if(ImGui::DragFloat2("Position", &spritePosition_.x, 1.0f)) {
		if(sprite_) {
			sprite_->SetPosition(spritePosition_);
		}
		if(spritePad_) {
			spritePad_->SetPosition(spritePosition_);
		}
	}
	if(ImGui::DragFloat2("Scale", &spriteScale_.x, 1.0f)) {
		if(sprite_) {
			sprite_->SetScale(spriteScale_);
		}
		if(spritePad_) {
			spritePad_->SetScale(spriteScale_);
		}
	}
	ImGui::DragFloat("PopIn Duration", &popInDuration_, 0.01f, 0.0f, 2.0f);
	ImGui::DragFloat("PopOut Duration", &popOutDuration_, 0.01f, 0.0f, 2.0f);

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
	editTexturePath("Teeth Texture", teethTexturePath_);
	editTexturePath("Open Texture", openTexturePath_);
	editTexturePath("Choco Texture", chocoTexturePath_);
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
	if(derived->contains("teethTexture")) {
		teethTexturePath_ = derived->at("teethTexture").get<std::string>();
	}
	if(derived->contains("openTexture")) {
		openTexturePath_ = derived->at("openTexture").get<std::string>();
	}
	if(derived->contains("chocoTexture")) {
		chocoTexturePath_ = derived->at("chocoTexture").get<std::string>();
	}
	if(derived->contains("spritePosition")) {
		spritePosition_ = derived->at("spritePosition").get<CalyxEngine::Vector2>();
	}
	if(derived->contains("spriteScale")) {
		spriteScale_ = derived->at("spriteScale").get<CalyxEngine::Vector2>();
	}
	if(derived->contains("popInDuration")) {
		popInDuration_ = derived->at("popInDuration").get<float>();
	}
	if(derived->contains("popOutDuration")) {
		popOutDuration_ = derived->at("popOutDuration").get<float>();
	}
}

void TutorialEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventData_.ExtractParamsToJson(derived["eventData"]);
	derived["jumpTexture"]	  = jumpTexturePath_;
	derived["diveTexture"]	  = diveTexturePath_;
	derived["purposeTexture"] = purposeTexturePath_;
	derived["teethTexture"]	  = teethTexturePath_;
	derived["openTexture"]	  = openTexturePath_;
	derived["chocoTexture"]	  = chocoTexturePath_;
	derived["spritePosition"] = spritePosition_;
	derived["spriteScale"]	  = spriteScale_;
	derived["popInDuration"]  = popInDuration_;
	derived["popOutDuration"] = popOutDuration_;
}

void TutorialEvent::RemapSceneObjectReferences([[maybe_unused]] const std::unordered_map<Guid, Guid>& guidMap) {
}
