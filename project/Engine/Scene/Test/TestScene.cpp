

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
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Application/System/Environment.h>
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

	std::string scenePath = "Resources/Assets/Scenes/test.scene";
	SceneSerializer::Load(*sceneContext_, scenePath);
	sceneContext_->SetScenePath(scenePath);

	LoadAssets();

	stage_ = std::make_unique<Stage>();
	stage_->Initialize(20.0f, 60.0f);

	stageGimmickManager_ = std::make_unique<StageGimmickManager>();
	stageGimmickManager_->Initialize();

	ShockwaveManager::GetInstance()->SetStage(stage_.get());

	//=========================
	// グラフィック関連
	//=========================

	InitPauseResource();

	transitionControl_ = std::make_unique<TransitionControl>();
	transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds");
	// シーンタイプに基づいて自動で演出をセット
	transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::TEST);
	transitionControl_->StartOpening(0.5f, [this]() {
		IsOpening_ = false;
	});
	IsOpening_ = true;
	IsPhase_   = false;
	isPaused_ = false;
	isFanOpen_	   = false;
	selectedIndex_ = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update([[maybe_unused]]float dt){

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) return;

	// ポーズの切り替え
	if(CalyxFoundation::Input::TriggerKey(DIK_ESCAPE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::START)) {
		if(!isPaused_) {
			lastDt_	  = dt;
			isPaused_ = true;
			ClockManager::GetInstance()->SetTimeScale(0.0f);
			PauseOpen();
		} else {
			PauseClose();
		}
	}

	CheckStageState(dt);

	if (isPaused_) {
		PauseUIUpdate(lastDt_);
		PauseUpdate(lastDt_);
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
		fanBg_->Draw(spriteRenderer_.get());
		spriteRenderer_->Register(resumeBtn_.get());
		spriteRenderer_->Register(toSelectBtn_.get());
		spriteRenderer_->Register(toTitleBtn_.get());
	}
	
	transitionControl_->Draw(spriteRenderer_.get());

	//シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);

}

void TestScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TestScene::InitPauseResource() {
	const std::string whiteTex = "Textures/white1x1.dds";
	resumeBtn_				   = std::make_unique<Sprite>(whiteTex);
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

	fanBg_	 = std::make_unique<CalyxEngine::SpriteObject2d>();
	fanAnim_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	fanAnim_->Bind(fanBg_.get());
	fanBg_->Initialize("Textures/Pause/fan.png");
	fanBg_->SetPosition({0.0f, 720.0f});
	fanBg_->SetScale({1280.0f, 720.0f});
	fanBg_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	auto asset = std::make_shared<CalyxEngine::SpriteAnimationAsset>();
	asset->division	   = {7, 1}; // 例: 4x4分割
	asset->texturePath = "Textures/Pause/fan.png";

	CalyxEngine::SpriteAnimationClip clip;
	clip.name		  = "FanOpen";
	clip.startFrame	  = 0;
	clip.frameCount	  = 7;
	clip.frameDuration = 0.6f / 7.0f;
	clip.loop		  = false;
	asset->clips.push_back(clip);

	fanAnim_->SetAnimationAsset(asset);

	isOncePlay_	   = false;
	openingTime_   = 1.0f;
	frameDuration_ = 0.6f / 7.0f;
	frame_		   = 0;
}

void TestScene::CheckStageState([[maybe_unused]] float dt) {
	if(stage_->IsClear()) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::CLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR), std::move(payload_));
		});
	}
	if(stage_->IsGameOver()) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::CLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER), std::move(payload_));
		});
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_9)) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::CLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::CLEAR), std::move(payload_));
		});
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_8)) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::CLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER), std::move(payload_));
		});
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
		PauseClose();
	});

	updateBtn(toSelectBtn_, 1, [&]() {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::SELECT);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT), std::move(payload_));
		});
	});

	updateBtn(toTitleBtn_, 2, [&]() {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::TITLE);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE), std::move(payload_));
		});
	});
}

void TestScene::PauseUIUpdate(float dt) {
	currentOpeningTime_ += dt;
	float t = (std::min)(currentOpeningTime_ / openingTime_, 1.0f);

	float h		= static_cast<float>(kWindowHeight) * 1.75f;
	float y		= 0.0f;
	int	  frame = 0;

	if(isFanOpen_) {
		// t:0~0.4は下からfanBg_が登ってくる（ｙが720から０まで）
		if(t <= 0.4f) {
			float upT = (std::min)(t / 0.4f, 1.0f);
			float easeT = std::sqrtf(1.0f - std::powf(upT - 1.0f, 2));
			y			= h * (1.0f - easeT);
			frame	  = 0;
			isOncePlay_ = false;
		} else {
			if(!fanAnim_->IsPlaying() && !isOncePlay_) {
				isOncePlay_ = true;
				fanAnim_->SetReversed(false);
				fanAnim_->Play("FanOpen");
			}
			fanAnim_->Update(dt);
		}
	} else {
		// isFanOpen_じゃない時は
		// t:0~0.6はfanBg_が閉じる（1280＊720の連番（横1280＊７、縦７２０））
		if(t <= 0.6f) {
			if(!fanAnim_->IsPlaying() && !isOncePlay_) {
				isOncePlay_ = true;
				fanAnim_->SetReversed(true);
				fanAnim_->Play("FanOpen");
			}
			fanAnim_->Update(dt);
		} else {
			// t:0.6~1はfanBg_が下がっていく（ｙが0から720まで）
			float downT = (std::min)((t - 0.6f) / 0.4f, 1.0f);
			float easeT = 1.0f - std::sqrtf(1.0f - std::powf(downT, 2));
			y			= h * easeT;
			frame		= 0;
		}
	}

	fanBg_->SetPosition({0.0f, y});

	//// UVアニメーション (横1280*7の連番)
	//float frameWidth = 1.0f / 7.0f;
	//fanBg_->SetUvScale({frameWidth, 1.0f});
	//fanBg_->SetUvOffset({static_cast<float>(frame) * frameWidth, 0.0f});

	fanBg_->Update(dt);
	// 閉じきったらポーズ終了
	if(isFanOpen_) {
		return;
	}
	if(t >= 1.0f) {
		isPaused_ = false;
		ClockManager::GetInstance()->SetTimeScale(1.0f);
	}
}

void TestScene::PauseOpen() {
	isFanOpen_ = true;
	currentOpeningTime_ = 0.0f;
}

void TestScene::PauseClose() {
	if(!isFanOpen_) return;
	isOncePlay_			= false;
	isFanOpen_			= false;
	currentOpeningTime_ = 0.0f;
}

std::unique_ptr<TransitionPayload> TestScene::BuildNowTypePayload(SceneType Type) {
	auto payload  = std::make_unique<TransitionPayload>();
	payload->type = Type;
	return payload;
}

void TestScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<GameTransitionPayload*>(payload.get())) {
		stageNum_ = p->stageNum_;
	}
}