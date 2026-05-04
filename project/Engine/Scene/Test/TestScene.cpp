

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>

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
void TestScene::LoadAssets(){}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Initialize(){
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("TestScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/test.scene");

	LoadAssets();

	stageGimmickManager_ = std::make_unique<StageGimmickManager>();
	stageGimmickManager_->Initialize();

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
	if (CalyxFoundation::Input::TriggerKey(DIK_0)) {
		isPaused_ = !isPaused_;
		if(isPaused_) {
			ClockManager::GetInstance()->SetTimeScale(0.0f);
		} else {
			ClockManager::GetInstance()->SetTimeScale(1.0f);
		}
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_9)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR));
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_8)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER));
	}

	if (isPaused_) {
		// キー入力による選択変更
		if (CalyxFoundation::Input::TriggerKey(DIK_W)) {
			selectedIndex_ = (selectedIndex_ - 1 + 3) % 3;
		}
		if (CalyxFoundation::Input::TriggerKey(DIK_S)) {
			selectedIndex_ = (selectedIndex_ + 1) % 3;
		}

		bool isConfirmed = CalyxFoundation::Input::TriggerKey(DIK_SPACE);

		// ボタンの更新処理
		auto updateBtn = [&](std::unique_ptr<Sprite>& btn, int index, std::function<void()> onClick) {
			if (selectedIndex_ == index) {
				btn->SetColor({0.8f, 0.8f, 0.3f, 1.0f}); // 選択中は黄色っぽく
				if (isConfirmed) {
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

		return;
	}


	stageGimmickManager_->ShowGui();
	stageGimmickManager_->Update(dt);

	//衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TestScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget*  rt){

	//========================================================//
	//	spriteの登録
	//========================================================//
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