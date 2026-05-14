

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

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/Title.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({640.0f, 0.0f}, {320.0f, 180.0f});
	pauseBg_->SetAnchorPoint({0.5f, 0.0f});
	pauseBg_->SetColor({0.0f, 1.0f, 0.0f, 1.0f});
	pauseBg_->Update();

	phaseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	phaseBg_->Initialize({1280.0f, 0.0f}, {1280.0f, 720.0f});
	phaseBg_->SetAnchorPoint({0.f, 0.0f});
	phaseBg_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	phaseBg_->Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Update([[maybe_unused]] float dt) {

	PhaseUpdate(dt);

	if(IsPhase_) return;

	if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		IsPhase_ = true;
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TitleScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	spriteの登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());

	if(IsPhase_) {
		spriteRenderer_->Register(phaseBg_.get());
	}

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void TitleScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TitleScene::PhaseUpdate(float dt) {
	if(!IsPhase_) return;

	CalyxEngine::Vector2 pos = phaseBg_->GetPosition();
	pos.x -= 2500.0f * dt;
	if(pos.x <= 0.0f) {
		pos.x = 0.0f;
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT));
	}
	phaseBg_->SetPosition(pos);
	phaseBg_->Update();
}