

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/DemoShockwave/ShockwaveManager.h>
#include <Game/StageGimmick/Tutorial/TutorialEvent.h>
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
#include <externals/nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <algorithm>

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

	std::string scenePath;
	if(stageNum_ == 0) {
		scenePath = "Resources/Assets/Scenes/test.scene";
	} else {
		scenePath = "Resources/Assets/Scenes/Stage_" + std::to_string(stageNum_) + ".scene";
	}
	SceneSerializer::Load(*sceneContext_, scenePath);
	sceneContext_->SetScenePath(scenePath);

	LoadAssets();

	LoadStageSetting();

	stage_ = std::make_unique<Stage>();
	stage_->Initialize(stageHP_, stageLimitTime_);

	stageGimmickManager_ = std::make_unique<StageGimmickManager>();
	stageGimmickManager_->Initialize();

	player_ = sceneContext_->FindFirst<DemoPlayer>();
	playerUI_ = std::make_unique<PlayerUI>();
	if (player_) {
		playerUI_->Initialize(player_->GetMaxHP());
	} else {
		playerUI_->Initialize(4); // デフォルト
	}

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
	IsOpening_	   = true;
	IsPhase_	   = false;
	isPaused_	   = false;
	isFanOpen_	   = false;
	isFadeFinish_  = false;
	selectedIndex_ = 0;
	fadeTime_	   = 0.2f;
	ClockManager::GetInstance()->SetTimeScale(1.0f);
	buttunAudio_.Load("Buttun");
	timeUpAudio_.Load("timeOver");
	bgmAudio_.Load("Game");
	AudioAPI::Play(bgmAudio_, true, 0.3f);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update([[maybe_unused]]float dt){

	transitionControl_->Update(dt);

	if(player_) {
		playerUI_->Update(dt, player_->GetLife());
	}

	if(IsPhase_ || IsOpening_) return;

	// ポーズの切り替え
	if(CalyxFoundation::Input::TriggerKey(DIK_ESCAPE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::START)) {
		if(!isPaused_ && dt >= 0.0f) {
			lastDt_	  = dt;
			isPaused_ = true;
			ClockManager::GetInstance()->SetTimeScale(0.0f);
			PauseOpen();
		} else {
			if(currentOpeningTime_ >= openingTime_ && fanAnim_->IsFinished()) {
				PauseClose();
			}
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

#ifdef DEVELOP
	if(ImGui::Begin("StageSetting")) {
		// ステージ番号を1~5で切り替え
		if(ImGui::SliderInt("stageNum", &stageNum_, kMinStageNum_, kMaxStageNum_)) {
			LoadStageSetting();
		}
		ImGui::Text("%s", settingPath_.c_str());
		ImGui::DragFloat("hp", &stageHP_, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("limitTime", &stageLimitTime_, 0.1f, 0.0f, 120.0f);
		if(ImGui::Button("Save")) {
			SaveStageSetting();
		}
	}
	ImGui::End();
#endif

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

	if(sceneContext_) {
		for(auto& obj : sceneContext_->GetObjectLibrary()->FindByClassName("TutorialEvent")) {
			if(auto* tutorial = dynamic_cast<TutorialEvent*>(obj.get())) {
				tutorial->DrawSprite(spriteRenderer_.get());
			}
		}
	}

	if (playerUI_) {
		playerUI_->Draw(spriteRenderer_.get());
	}

	if(isPaused_) {
		fanBg_->Draw(spriteRenderer_.get());
		pauseUI_->Draw(spriteRenderer_.get());
		for(auto& cloud : cloud_) {
			cloud->Draw(spriteRenderer_.get());
		}
		resumeBtn_->Draw(spriteRenderer_.get());
		toRetryBtn_->Draw(spriteRenderer_.get());
		toSelectBtn_->Draw(spriteRenderer_.get());
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

void TestScene::LoadStageSetting() {
	stageNum_	 = std::clamp(stageNum_, kMinStageNum_, kMaxStageNum_);
	settingPath_ = "Resources/Params/Game/StagesSettings/" + std::to_string(stageNum_) + "_Setting.json";

	std::ifstream settingIfs(settingPath_);
	if(settingIfs) {
		nlohmann::json settingJson;
		settingIfs >> settingJson;
		stageHP_		= settingJson.value("hp", stageHP_);
		stageLimitTime_ = settingJson.value("limitTime", stageLimitTime_);
	} else {
		// ファイルが無ければデフォルト値
		SaveStageSetting();
	}
}

void TestScene::SaveStageSetting() {
	if(settingPath_.empty()) {
		return;
	}

	std::filesystem::create_directories(
		std::filesystem::path(settingPath_).parent_path());

	nlohmann::json settingJson;
	settingJson["hp"]		 = stageHP_;
	settingJson["limitTime"] = stageLimitTime_;

	std::ofstream settingOfs(settingPath_);
	if(settingOfs) {
		settingOfs << std::setw(4) << settingJson << std::endl;
	}
}

void TestScene::InitPauseResource() {
	pauseUI_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	pauseUI_->Initialize("Textures/UI/Pose/pose_text.png");
	pauseUI_->SetPosition({640.0f, 110.0f});
	pauseUI_->SetScale({640.0f, 180.0f});
	pauseUI_->SetAnchorPoint({0.5f, 0.5f});
	pauseUI_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	resumeBtn_				   = std::make_unique<CalyxEngine::SpriteObject2d>();
	resumeBtn_->Initialize("Textures/UI/Pose/game.png");
	resumeBtn_->SetPosition({640.0f, 210.0f});
	resumeBtn_->SetScale({640.0f, 180.0f});
	resumeBtn_->SetAnchorPoint({0.5f, 0.5f});
	resumeBtn_->SetColor({0.3f, 0.3f, 0.3f, 0.0f});

	toRetryBtn_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	toRetryBtn_->Initialize("Textures/UI/Pose/retry.png");
	toRetryBtn_->SetPosition({640.0f, 310.0f});
	toRetryBtn_->SetScale({640.0f, 180.0f});
	toRetryBtn_->SetAnchorPoint({0.5f, 0.5f});
	toRetryBtn_->SetColor({0.3f, 0.3f, 0.3f, 0.0f});

	toSelectBtn_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	toSelectBtn_->Initialize("Textures/UI/Pose/select.png");
	toSelectBtn_->SetPosition({640.0f, 410.0f});
	toSelectBtn_->SetScale({640.0f, 180.0f});
	toSelectBtn_->SetAnchorPoint({0.5f, 0.5f});
	toSelectBtn_->SetColor({0.3f, 0.3f, 0.3f, 0.0f});

	for(int i = 0; i < 4; i++) {
		auto cloud = std::make_unique<CalyxEngine::SpriteObject2d>();
		if(i == 0) {
			cloud->Initialize("Textures/Pause/cloud01.png");
			cloud->SetPosition({170.0f, 300.0f});
		}
		if(i == 1) {
			cloud->Initialize("Textures/Pause/cloud02.png");
			cloud->SetPosition({1080.0f, 140.0f});
		}
		if(i == 2) {
			cloud->Initialize("Textures/Pause/cloud03.png");
			cloud->SetPosition({300.0f, 500.0f});
		}
		if(i == 3) {
			cloud->Initialize("Textures/Pause/cloud04.png");
			cloud->SetPosition({1100.0f, 530.0f});
		}
		cloud->SetScale({320.0f, 180.0f});
		cloud->SetAnchorPoint({0.5f, 0.5f});
		cloud->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
		cloud_.push_back(std::move(cloud));
	}

	fanBg_	 = std::make_unique<CalyxEngine::SpriteObject2d>();
	fanAnim_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	fanAnim_->Bind(fanBg_.get());
	fanBg_->Initialize("Textures/Pause/fan.png");
	fanBg_->SetPosition({0.0f, 720.0f});
	fanBg_->SetScale({1280.0f, 720.0f});
	fanBg_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	auto asset = std::make_shared<CalyxEngine::SpriteAnimationAsset>();
	asset->division	   = {6, 1}; // 例: 4x4分割
	asset->texturePath = "Textures/Pause/fan.png";

	CalyxEngine::SpriteAnimationClip clip;
	clip.name		  = "FanOpen";
	clip.startFrame	  = 0;
	clip.frameCount	  = 6;
	clip.frameDuration = 0.6f / 6.0f;
	clip.loop		  = false;
	asset->clips.push_back(clip);

	fanAnim_->SetAnimationAsset(asset);

	isOncePlay_	   = false;
	openingTime_   = 1.0f;
}

void TestScene::CheckStageState([[maybe_unused]] float dt) {
	if(stage_->IsClear() || player_->GetIsTutorialClear()) {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::PRECLEAR);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::PRECLEAR), std::move(payload_));
		});
	}
	if(stage_->IsGameOver() || player_->GetLife() <= 0) {
		AudioAPI::Play(timeUpAudio_, false, 0.3f);
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::GAMEOVER);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::GAMEOVER), std::move(payload_));
		});
	}
}

void TestScene::PauseUpdate([[maybe_unused]] float dt) {
	// 入力による選択変更
	if(!isFanOpen_ || !isFadeFinish_) {
		return;
	}

	if(CalyxFoundation::Input::TriggerKey(DIK_W) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_UP)) {
		selectedIndex_ = (selectedIndex_ - 1 + 3) % 3;
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_S) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_DOWN)) {
		selectedIndex_ = (selectedIndex_ + 1) % 3;
	}

	bool isConfirmed = CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A);

	// ボタンの更新処理
	auto updateBtn = [&](std::unique_ptr<CalyxEngine::SpriteObject2d>& btn, int index, std::function<void()> onClick) {
		if(selectedIndex_ == index) {
			btn->SetColor({0.8f, 0.8f, 0.3f, 1.0f}); // 選択中は黄色っぽく
			if(isConfirmed) {
				AudioAPI::Play(buttunAudio_, false, 0.3f);
				onClick();
			}
		} else {
			btn->SetColor({0.3f, 0.3f, 0.3f, 1.0f});
		}
		btn->Update(dt); 
	};

	updateBtn(resumeBtn_, 0, [&]() {
		PauseClose();
	});

	updateBtn(toRetryBtn_, 1, [&]() {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildGamePayload(stageNum_);
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::TEST);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST), std::move(payload_));
		});
	});

	updateBtn(toSelectBtn_, 2, [&]() {
		ClockManager::GetInstance()->SetTimeScale(1.0f);
		payload_ = BuildNowTypePayload(SceneType::TEST);
		IsPhase_ = true;
		AudioAPI::Stop(bgmAudio_);
		transitionControl_->SetAutoPreset(SceneType::TEST, SceneType::SELECT);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT), std::move(payload_));
		});
	});
}

void TestScene::PauseUIUpdate(float dt) {
	if(isFanOpen_) {
		if(currentOpeningTime_ >= openingTime_ && fanAnim_->IsFinished()) {
			ButtonFadeIn(dt);
			CloudFade(dt);
		}
	} else {
		// 雲はボタンとは別タイマーで動かすため、フェード完了前でも毎フレーム進める
		CloudFade(dt);
		if(!isFadeFinish_) {
			ButtonFadeOut(dt);
			return;
		}
	}

	float t = (std::min)(currentOpeningTime_ / openingTime_, 1.0f);
	currentOpeningTime_ += dt;

	float h		= static_cast<float>(kWindowHeight);
	float y		= 0.0f;
	int	  frame = 0;

	if(isFanOpen_) {
		// t:0~0.7は下からfanBg_が登ってくる（ｙが720から０まで）
		if(t <= 0.7f) {
			float upT = (std::min)(t / 0.7f, 1.0f);
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
		// isFanOpen_じゃない時
		if(t <= 0.3f) {
			if(!fanAnim_->IsPlaying() && !isOncePlay_) {
				isOncePlay_ = true;
				fanAnim_->SetReversed(true);
				fanAnim_->Play("FanOpen");
			}
		} else {
			// t:0.3~1はfanBg_が下がっていく（ｙが0から720まで）
			float downT = (std::min)((t - 0.3f) / 0.7f, 1.0f);
			float easeT = 1.0f - std::sqrtf(1.0f - std::powf(downT, 2));
			y			= h * easeT;
			frame		= 0;
		}
		fanAnim_->Update(dt);
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
	if(t >= 1.0f && fanAnim_->IsFinished()) {
		isPaused_ = false;
		ClockManager::GetInstance()->SetTimeScale(1.0f);
	}
}

void TestScene::PauseOpen() {
	isFanOpen_			   = true;
	isFadeFinish_		   = false;
	currentOpeningTime_	   = 0.0f;
	currentFadeTime_	   = 0.0f;
	currentCloudFadeTime_  = 0.0f;
}

void TestScene::PauseClose() {
	if(!isFanOpen_) return;
	isOncePlay_			   = false;
	isFanOpen_			   = false;
	isFadeFinish_		   = false;
	currentOpeningTime_	   = 0.0f;
	currentFadeTime_	   = 0.0f;
	currentCloudFadeTime_  = 0.0f;
}

void TestScene::ButtonFadeIn(float dt) {
	if(isFadeFinish_) return;
	float t = (std::min)(currentFadeTime_ / fadeTime_, 1.0f);
	currentFadeTime_ += dt;

	float alpha = t;
	pauseUI_->SetAlpha(alpha);
	resumeBtn_->SetAlpha(alpha);
	toRetryBtn_->SetAlpha(alpha);
	toSelectBtn_->SetAlpha(alpha);
	pauseUI_->Update(dt);
	resumeBtn_->Update(dt);
	toRetryBtn_->Update(dt);
	toSelectBtn_->Update(dt);

	if(t >= 1.0f) {
		isFadeFinish_ = true;
	}
}

void TestScene::ButtonFadeOut(float dt) {
	if(isFadeFinish_) return;
	float t = (std::min)(currentFadeTime_ / fadeTime_, 1.0f);
	currentFadeTime_ += dt;

	float alpha = 1.0f - t;
	pauseUI_->SetAlpha(alpha);
	resumeBtn_->SetAlpha(alpha);
	toRetryBtn_->SetAlpha(alpha);
	toSelectBtn_->SetAlpha(alpha);
	pauseUI_->Update(dt);
	resumeBtn_->Update(dt);
	toRetryBtn_->Update(dt);
	toSelectBtn_->Update(dt);

	if(t >= 1.0f) {
		isFadeFinish_ = true;
	}
}

void TestScene::CloudFade(float dt) {
	currentCloudFadeTime_ += dt;

	float alpha = 0.0f;
	if(isFanOpen_) {
		// 開く：ボタンがある程度現れてから、ボタンより時間をかけてゆっくりα→1
		float t = (currentCloudFadeTime_ - cloudFadeInDelay_) / cloudFadeInTime_;
		t	  = (std::max)(0.0f, (std::min)(t, 1.0f));
		alpha = t;
	} else {
		// 閉じる：ボタンとタイミングをずらしてα→0（閉じきる前に0になってもよい）
		float t = (currentCloudFadeTime_ - cloudFadeOutDelay_) / cloudFadeOutTime_;
		t	  = (std::max)(0.0f, (std::min)(t, 1.0f));
		alpha = 1.0f - t;
	}

	for(auto& cloud : cloud_) {
		cloud->SetAlpha(alpha);
		cloud->Update(dt);
	}
}

std::unique_ptr<TransitionPayload> TestScene::BuildNowTypePayload(SceneType Type) {
	auto payload  = std::make_unique<TransitionPayload>();
	payload->type = Type;
	return payload;
}

std::unique_ptr<GameTransitionPayload> TestScene::BuildGamePayload(int num) {
	auto payload	   = std::make_unique<GameTransitionPayload>();
	payload->stageNum_ = num;
	payload->type_	   = SceneType::TEST;
	return payload;
}

void TestScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<GameTransitionPayload*>(payload.get())) {
		stageNum_ = p->stageNum_;
		preType_ = p->type_;
	}
}