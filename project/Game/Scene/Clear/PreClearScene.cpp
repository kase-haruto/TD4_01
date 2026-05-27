

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Clear/PreClearScene.h>

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
PreClearScene::PreClearScene() {
	// シーン名を設定
	BaseScene::SetSceneName("PreClearScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void PreClearScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void PreClearScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("PreClearScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/PreClearScene.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({0.0f, 0.0f}, {320.0f, 180.0f});
	pauseBg_->SetColor({1.0f, 1.0f, 0.0f, 1.0f});
	pauseBg_->Update();

	transitionControl_ = std::make_unique<TransitionControl>();
	transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds");
	// シーンタイプに基づいて自動で演出をセット
	transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::PRECLEAR);
	transitionControl_->StartOpening(0.5f, [this]() {
		IsOpening_ = false;
	});
	IsOpening_ = true;
	IsPhase_   = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void PreClearScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) {
		return;
	}

	AnimUpdate(dt);

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void PreClearScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	spriteの登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());

	transitionControl_->Draw(spriteRenderer_.get());
	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void PreClearScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void PreClearScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<TransitionPayload*>(payload.get())) {
		preType_ = p->type;
	}
}

std::unique_ptr<TransitionPayload> PreClearScene::BuildNowTypePayload(SceneType Type) {
	auto payload  = std::make_unique<TransitionPayload>();
	payload->type = Type;
	return payload;
}

void PreClearScene::AnimUpdate(float dt) {
	animTime_ -= dt;

	if(animTime_ < 0.0f) {
		IsPhase_ = true;
		payload_ = BuildNowTypePayload(SceneType::PRECLEAR);
		transitionControl_->SetAutoPreset(SceneType::PRECLEAR, SceneType::CLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR), std::move(payload_));
		});
	}
}
