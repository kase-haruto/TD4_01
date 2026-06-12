

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Select/SelectScene.h>
#include <Game\Scene\Transition\TransitionPayload.h>

// engine
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
SelectScene::SelectScene() {
	// シーン名を設定
	BaseScene::SetSceneName("SelectScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("SelectScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/SelectScene.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================

	transitionControl_ = std::make_unique<TransitionControl>();
	if(preType_ == SceneType::TITLE) {
		transitionControl_->Initialize("Textures/Transition/wave.png", "Textures/Transition/wave.png");
		// シーンタイプに基づいて自動で演出をセット
		transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::SELECT);
		transitionControl_->StartOpening(2.0f, [this]() {
			IsOpening_ = false;
			transitionControl_->SetTexturePlate1("Textures/uvChecker.dds");
			transitionControl_->SetTexturePlate2("Textures/uvChecker.dds");
		});
	} else {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds");
		// シーンタイプに基づいて自動で演出をセット
		transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::SELECT);
		transitionControl_->StartOpening(0.5f, [this]() {
			IsOpening_ = false;
			transitionControl_->SetTexturePlate1("Textures/uvChecker.dds");
			transitionControl_->SetTexturePlate2("Textures/uvChecker.dds");
		});
	}
	IsOpening_ = true;
	IsPhase_   = false;

	shoujiOpenTime_ = 2.0f;
	isShoujiOpen_ = false;
	isEat_			= false;

	oni_ = std::static_pointer_cast<GeneralObject>(sceneContext_->GetObjectLibrary()->FindByName("oni"));
	if(oni_) {
		// oni のアニメーションはループさせず、決定するまでは先頭で止めておく
		if(auto* anim = oni_->AnimationModel()) {
			anim->SetCurrentLoop(false);
			anim->ResetCurrentTime();
			anim->SetCurrentSpeed(0.0f);
		}
	}

	left_ = std::make_unique<Sprite>("Textures/UI/Select/arrow_left.png");
	left_->Initialize({80.0f, 360.0f}, {160.0f, 90.0f});
	left_->SetAnchorPoint({0.5f, 0.5f});
	left_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	left_->Update();

	right_ = std::make_unique<Sprite>("Textures/UI/Select/arrow_right.png");
	right_->Initialize({1200.0f, 360.0f}, {160.0f, 90.0f});
	right_->SetAnchorPoint({0.5f, 0.5f});
	right_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	right_->Update();

	pressButton_ = std::make_unique<Sprite>("Textures/UI/Select/selectUI_cont.png");
	pressButton_->Initialize({640.0f, 450.0f}, {640.0f, 360.0f});
	pressButton_->SetAnchorPoint({0.5f, 0.0f});
	pressButton_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	pressButton_->Update();

	pressKey_ = std::make_unique<Sprite>("Textures/UI/Select/selectUI_key.png");
	pressKey_->Initialize({640.0f, 450.0f}, {640.0f, 360.0f});
	pressKey_->SetAnchorPoint({0.5f, 0.0f});
	pressKey_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	pressKey_->Update();

	openAudio_.Load("syouji_open");
	eatAudio_.Load("eat");
	bgmAudio_.Load("select_2");
	buttunAudio_.Load("Buttun");
	AudioAPI::Play(bgmAudio_, true, 0.3f);

	isPad_ = CalyxFoundation::Input::IsGamepadConnected();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	UpdateInputDevice();

	if(IsPhase_ || IsOpening_) {
		return;
	}

	SelectUpdate(dt);

	if(CalyxFoundation::Input::PushKey(DIK_LCONTROL)&&CalyxFoundation::Input::TriggerKey(DIK_0)) {
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::TITLE);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE));
		});
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void SelectScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	sprite の 登録
	//========================================================//
	if(!isShoujiOpen_) {
		spriteRenderer_->Register(left_.get());
		spriteRenderer_->Register(right_.get());

		if(isPad_) {
			spriteRenderer_->Register(pressButton_.get());
		} else {
			spriteRenderer_->Register(pressKey_.get());
		}
	}

	transitionControl_->Draw(spriteRenderer_.get());

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void SelectScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void SelectScene::SelectUpdate(float dt) {
	if(isShoujiOpen_) {
		ShoujiOpen(dt);
		return;
	}

	const int maxStages = 5;

	Camera3d*			 cam = CameraManager::GetMain3d();
	CalyxEngine::Vector3 pos = cam->GetTranslate();

	// 左右入力によるステージ選択
	if(CalyxFoundation::Input::TriggerKey(DIK_A) || CalyxFoundation::Input::TriggerKey(DIK_LEFT) ||
	   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_LEFT)) {
		selectedIndex_ = (selectedIndex_ - 1 + maxStages) % maxStages;
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_D) || CalyxFoundation::Input::TriggerKey(DIK_RIGHT) ||
	   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_RIGHT)) {
		selectedIndex_ = (selectedIndex_ + 1) % maxStages;
	}

	float targetX = cameraBaseX_ + selectedIndex_ * cameraSpacing_;
	float t = 1.0f - std::exp(-cameraLerpRate_ * dt);
	pos.x += (targetX - pos.x) * t;
	cam->SetCamera(pos, cam->GetRotate());

	const float settleThreshold = 0.05f;
	bool		cameraSettled	= std::fabs(targetX - pos.x) <= settleThreshold;

	// 決定操作
	if(cameraSettled && CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		const std::string suffix = std::to_string(selectedIndex_ + 1);

		// 選択中ステージの障子(左右)を取得して初期Xを記録
		stageFrame_ = sceneContext_->GetObjectLibrary()->FindByName("Stage_" + suffix);
		if(stageFrame_) {
			for(auto& child : stageFrame_->GetChildren()) {
				if(child->GetName() == "ShoujiL_" + suffix) {
					shoujiL_ = child;
					lBase_	 = shoujiL_->GetWorldTransform().translation.x;
				} else if(child->GetName() == "ShoujiR_" + suffix) {
					shoujiR_ = child;
					rBase_	 = shoujiR_->GetWorldTransform().translation.x;
				}
			}
		}

		// 障子オープンと同時に、選んだ障子の正面へ
		if(oni_) {
			if(shoujiL_ && shoujiR_) {
				CalyxEngine::Vector3 oniPos = oni_->GetWorldTransform().translation;
				CalyxEngine::Vector3 framePos;
				if(stageFrame_) {
					framePos = stageFrame_->GetWorldTransform().translation;
				}

				oniPos.x = framePos.x;
				oni_->SetTranslate(oniPos);
			}
			if(auto* anim = oni_->AnimationModel()) {
				anim->ResetCurrentTime();
				anim->SetCurrentSpeed(1.0f);
			}
		}

		// 遷移用ペイロードを用意し、障子オープン演出へ移行(selectedIndex_が0~4のため＋１)
		gamePayload_  = BuildGamePayload(selectedIndex_ + 1);
		isShoujiOpen_ = true;
		AudioAPI::Play(buttunAudio_, false, 0.3f);
		AudioAPI::Play(openAudio_, false, 0.5f);
	}
}

void SelectScene::PhaseUpdate(float) {

}

void SelectScene::ShoujiOpen(float dt) {
	float t = 1.0f - std::exp(-openRate_ * dt);

	// 左の障子は -X、右の障子は +X へスライド
	if(shoujiL_) {
		auto& lt	   = shoujiL_->GetWorldTransform().translation;
		float targetLX = lBase_ - openDistance_;
		lt.x += (targetLX - lt.x) * t;
	}
	if(shoujiR_) {
		auto& rt	   = shoujiR_->GetWorldTransform().translation;
		float targetRX = rBase_ + openDistance_;
		rt.x += (targetRX - rt.x) * t;
	}

	// 鬼のアニメーションが終了したら遷移
	bool oniFinished = false;
	if(oni_) {
		if(auto* anim = oni_->AnimationModel()) {
			shoujiOpenTime_ -= dt;
			if(!isEat_ && shoujiOpenTime_ < -3.4f) {
				isEat_ = true;
				AudioAPI::Play(eatAudio_, false, 0.25f);
			}
			oniFinished = anim->IsAnimationFinished();
		}
	} else {
		shoujiOpenTime_ -= dt;
		oniFinished = (shoujiOpenTime_ <= 0.0f);
	}

	if(oniFinished) {
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::SELECT, SceneType::TEST);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST), std::move(gamePayload_));
		});
	}
}

void SelectScene::UpdateInputDevice() {
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

std::unique_ptr<GameTransitionPayload> SelectScene::BuildGamePayload(int num) {
	auto payload = std::make_unique<GameTransitionPayload>();
	payload->stageNum_ = num;
	payload->type_ = SceneType::SELECT;
	return payload;
}

void SelectScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<TransitionPayload*>(payload.get())) {
		preType_ = p->type;
	}
}
