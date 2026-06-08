

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Title/TitleScene.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
TitleScene::TitleScene() {
	// シーン名を設定
	BaseScene::SetSceneName("TitleScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("TitleScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/TitleScene.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/Title/title.png");
	pauseBg_->Initialize({640.0f, -50.0f}, {960.0f, 540.0f});
	pauseBg_->SetAnchorPoint({0.5f, 0.0f});
	pauseBg_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	pauseBg_->Update();

	pressButton_ = std::make_unique<Sprite>("Textures/Title/titleUI_cont.png");
	pressButton_->Initialize({640.0f, 250.0f}, {640.0f, 360.0f});
	pressButton_->SetAnchorPoint({0.5f, 0.0f});
	pressButton_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	pressButton_->Update();

	pressKey_ = std::make_unique<Sprite>("Textures/Title/titleUI_key.png");
	pressKey_->Initialize({640.0f, 250.0f}, {640.0f, 360.0f});
	pressKey_->SetAnchorPoint({0.5f, 0.0f});
	pressKey_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	pressKey_->Update();

	transitionControl_ = std::make_unique<TransitionControl>();
	if(preType_ == SceneType::SELECT) {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/Transition/wave.png");
	} else {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds");
	}
	// シーンタイプに基づいて自動で演出をセット
	transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::TITLE);
	transitionControl_->StartOpening(0.5f, [this]() {
		IsOpening_ = false;
		transitionControl_->SetTexturePlate1("Textures/Transition/wave.png");
		transitionControl_->SetTexturePlate2("Textures/Transition/wave.png");
	});
	IsOpening_ = true;
	IsPhase_ = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) {
		return;
	}

	UpdateInputDevice();

	if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		IsPhase_ = true;
		payload_ = BuildNowTypePayload(SceneType::TITLE);
		transitionControl_->SetAutoPreset(SceneType::TITLE, SceneType::SELECT);
		transitionControl_->StartClosing(1.0f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT), std::move(payload_));
		});
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TitleScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	spriteの登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());

	if(isPad_) {
		spriteRenderer_->Register(pressButton_.get());
	} else {
		spriteRenderer_->Register(pressKey_.get());
	}

	transitionControl_->Draw(spriteRenderer_.get());

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void TitleScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TitleScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<TransitionPayload*>(payload.get())) {
		preType_ = p->type;
	}
}

void TitleScene::PhaseUpdate(float) {
}

void TitleScene::UpdateInputDevice() {
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
			return;
		}
	}

	// トリガー・スティックも「パッドに触った」として扱う
	constexpr float kTriggerThreshold = 0.1f;
	if(Input::GetLeftTrigger() > kTriggerThreshold ||
	   Input::GetRightTrigger() > kTriggerThreshold ||
	   Input::IsLeftStickMoved()) {
		isPad_ = true;
		return;
	}

	// キーボードに触れたらキーボード表示に戻す（任意：不要なら削除）
	for(uint32_t key = 0; key < 256; ++key) {
		if(Input::PushKey(key)) {
			isPad_ = false;
			return;
		}
	}
}

std::unique_ptr<TransitionPayload> TitleScene::BuildNowTypePayload(SceneType Type) {
	auto payload  = std::make_unique<TransitionPayload>();
	payload->type = Type;
	return payload;
}