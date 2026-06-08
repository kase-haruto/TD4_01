

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
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) {
		return;
	}

	SelectUpdate(dt);

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void SelectScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	sprite の 登録
	//========================================================//
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
		oni_ = sceneContext_->GetObjectLibrary()->FindByName("oni");

		// 遷移用ペイロードを用意し、障子オープン演出へ移行
		gamePayload_  = BuildGamePayload(selectedIndex_);
		isShoujiOpen_ = true;
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

	// 開き切るまでの時間をカウント
	shoujiOpenTime_ -= dt;
	if(shoujiOpenTime_ <= 0.0f) {
		// 開き終わったらシーン遷移を開始
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::SELECT, SceneType::TEST);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST), std::move(gamePayload_));
		});
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
