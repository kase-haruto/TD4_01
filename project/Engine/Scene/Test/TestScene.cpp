

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>
#include <Game/DemoPlayer/DemoPlayer.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Context/SceneContext.h>
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

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/TestScene.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================

	player_ = SceneAPI::Instantiate<DemoPlayer>("human.obj", "DemoPlayer");
	player_->Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update([[maybe_unused]]float dt){

	//衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TestScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget*  rt){

	//========================================================//
	//	spriteの登録
	//========================================================//
	// 
	// 
	//シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);

}

void TestScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}