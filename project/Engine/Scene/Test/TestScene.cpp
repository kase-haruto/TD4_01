

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/DemoShockwave/ShockwaveManager.h>
#include <Game\Scene\Game\GameTransitionPayload.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
TestScene::TestScene(){
	// シーン名を設定
	BaseScene::SetSceneName("TestScene");

}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Initialize(){
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("TestScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/test.scene");

	LoadAssets();

	stage_ = std::make_unique<Stage>();
	stage_->Initialize(20.0f, 60.0f);

	stageGimmickManager_ = std::make_unique<StageGimmickManager>();
	stageGimmickManager_->Initialize();

	ShockwaveManager::GetInstance()->SetStage(stage_.get());

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({0.0f, 0.0f}, {1280.0f, 720.0f});
	pauseBg_->SetColor({0.0f, 0.0f, 0.0f, 0.5f});
	pauseBg_->Update();

	const std::string whiteTex = "Textures/white1x1.dds";
	resumeBtn_ = std::make_unique<Sprite>(whiteTex);
	resumeBtn_->Initialize({640.0f, 200.0f}, {300.0f, 60.0f});
	resumeBtn_->SetAnchorPoint({0.5f, 0.5f});
	resumeBtn_->SetColor({0.3f, 0.3f, 0.3f, 1.0f});

	toSelectBtn_ = std::make_unique<Sprite>(whiteTex);
	toSelectBtn_->Initialize({640.0f, 350.0f}, {300.0f, 60.0f});
	toSelectBtn_->SetAnchorPoint({0.5f, 0.5f});
	toSelectBtn_->SetColor({0.3f, 0.3f, 0.3f, 1.0f});

	toTitleBtn_ = std::make_unique<Sprite>(whiteTex);
	toTitleBtn_->Initialize({640.0f, 500.0f}, {300.0f, 60.0f});
	toTitleBtn_->SetAnchorPoint({0.5f, 0.5f});
	toTitleBtn_->SetColor({0.3f, 0.3f, 0.3f, 1.0f});
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update([[maybe_unused]]float dt){
	// ポーズの切り替え
	if(CalyxFoundation::Input::TriggerKey(DIK_ESCAPE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::START)) {
		isPaused_ = !isPaused_;
		if(isPaused_) {
			ClockManager::GetInstance()->SetTimeScale(0.0f);
		} else {
			ClockManager::GetInstance()->SetTimeScale(1.0f);
		}
	}

	CheckStageState(dt);

	if (isPaused_) {
		PauseUpdate(dt);
		return;
	}

	ShockwaveManager::GetInstance()->CheckTakeDamageForStage();
	stage_->Update(dt);

	stageGimmickManager_->ShowGui();
	stageGimmickManager_->Update(dt);

	//衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TestScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget*  rt){

	//========================================================//
	//	spriteの登録
	//========================================================//
	stage_->Draw(spriteRenderer_.get());

	if (isPaused_) {
		spriteRenderer_->Register(pauseBg_.get());
		spriteRenderer_->Register(resumeBtn_.get());
		spriteRenderer_->Register(toSelectBtn_.get());
		spriteRenderer_->Register(toTitleBtn_.get());
	}
	
	//シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);

}

void TestScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TestScene::CheckStageState([[maybe_unused]] float dt) {
	if(stage_->IsClear()) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR));
	}
	if(stage_->IsGameOver()) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER));
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_9)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR));
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_8)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER));
	}
}

void TestScene::PauseUpdate([[maybe_unused]] float dt) {
	// 入力による選択変更
	if(CalyxFoundation::Input::TriggerKey(DIK_W) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_UP)) {
		selectedIndex_ = (selectedIndex_ - 1 + 3) % 3;
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_S) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_DOWN)) {
		selectedIndex_ = (selectedIndex_ + 1) % 3;
	}

	bool isConfirmed = CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A);

	// ボタンの更新処理
	auto updateBtn = [&](std::unique_ptr<Sprite>& btn, int index, std::function<void()> onClick) {
		if(selectedIndex_ == index) {
			btn->SetColor({0.8f, 0.8f, 0.3f, 1.0f}); // 選択中は黄色っぽく
			if(isConfirmed) {
				onClick();
			}
		} else {
			btn->SetColor({0.3f, 0.3f, 0.3f, 1.0f});
		}
		btn->Update(); 
	};

	updateBtn(resumeBtn_, 0, [&]() {
		isPaused_ = false;
		ClockManager::GetInstance()->SetTimeScale(1.0f);
	});

	updateBtn(toSelectBtn_, 1, [&]() {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT));
	});

	updateBtn(toTitleBtn_, 2, [&]() {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE));
	});
}

void TestScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<GameTransitionPayload*>(payload.get())) {
		stageNum_ = p->stageNum_;
	}
}