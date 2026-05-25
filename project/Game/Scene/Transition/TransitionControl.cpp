#include "TransitionControl.h"
#include <Engine/Application/System/Environment.h>
#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <algorithm>
#include <cmath>

TransitionControl::TransitionControl()	= default;
TransitionControl::~TransitionControl() = default;

void TransitionControl::Initialize(const std::string& texPath1, const std::string& texPath2) {
	plate1_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	plate1_->Initialize(texPath1);
	plate1_->SetPosition({0.0f, 0.0f});
	plate1_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->Update(0.0f);

	if(!texPath2.empty()) {
		plate2_ = std::make_unique<CalyxEngine::SpriteObject2d>();
		plate2_->Initialize(texPath2);
		plate2_->SetPosition({0.0f, 0.0f});
		plate2_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
		plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		plate2_->Update(0.0f);
	}

	state_ = TransitionState::Idle;
}

void TransitionControl::Update(float dt) {
	if(state_ == TransitionState::Idle || state_ == TransitionState::Full) return;

	timer_ += dt;
	float progress = (std::min)(timer_ / duration_, 1.0f);

	if(updateFunc_) {
		updateFunc_(progress, plate1_.get(), plate2_.get());
	}


	if(plateAnim1_) plateAnim1_->Update(dt);
	if(plateAnim2_) plateAnim2_->Update(dt);
	if(plateAnim3_) plateAnim3_->Update(dt);
	if(plateAnim4_) plateAnim4_->Update(dt);
	if(plate1_) plate1_->Update(dt);
	if(plate2_) plate2_->Update(dt);
	if(plate3_) plate3_->Update(dt);
	if(plate4_) plate4_->Update(dt);

	if(progress >= 1.0f) {
		auto cb				= onFinishedCallback_;
		onFinishedCallback_ = nullptr;

		if(state_ == TransitionState::Closing) {
			state_ = TransitionState::Full;
		} else {
			state_ = TransitionState::Idle;
		}

		if(cb) cb();
		if(plate1_) {
			plate1_->SetPosition({1280.0f, 0.0f});
			plate1_->SetAlpha(1.0f);
			plate1_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate1_->Update(dt);
		}
		if(plate2_) {
			plate2_->SetPosition({1280.0f, 0.0f});
			plate2_->SetAlpha(1.0f);
			plate2_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate2_->Update(dt);
		}
		if(plate3_) {
			plate3_->SetPosition({1280.0f, 0.0f});
			plate3_->SetAlpha(1.0f);
			plate3_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate3_->Update(dt);
		}
		if(plate4_) {
			plate4_->SetPosition({1280.0f, 0.0f});
			plate4_->SetAlpha(1.0f);
			plate4_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate4_->Update(dt);
		}
	}
}

void TransitionControl::Draw(SpriteRenderer* renderer) {
	if(state_ == TransitionState::Idle) return;

	if(plate4_ && isDrawPlate4_) plate4_->Draw(renderer);
	if(plate3_ && isDrawPlate3_) plate3_->Draw(renderer);
	if(plate2_ && isDrawPlate2_) plate2_->Draw(renderer);
	if(plate1_) plate1_->Draw(renderer);
}

void TransitionControl::StartClosing(float duration, std::function<void()> onCovered) {
	state_				= TransitionState::Closing;
	timer_				= 0.0f;
	duration_			= duration;
	onFinishedCallback_ = onCovered;
}

void TransitionControl::StartOpening(float duration, std::function<void()> onOpened) {
	state_				= TransitionState::Opening;
	timer_				= 0.0f;
	duration_			= duration;
	onFinishedCallback_ = onOpened;
}

void TransitionControl::SetPresetSlide(bool toRight) {
	plate1_->Update(0.0f);
	SetUpdateFunc([toRight, this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2) {
		p2;
		float w = static_cast<float>(kWindowWidth);
		float x = 0.0f;

		if(state_ == TransitionState::Closing) {
			// 画面外から画面内へ
			x = toRight ? -w + w * progress : w - w * progress;
		} else {
			// 画面内から画面外へ
			x = toRight ? w * progress : -w * progress;
		}
		p1->SetPosition({x, 0});
	});
}

void TransitionControl::SetPresetFade() {
	plate1_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	plate1_->Update(0.0f);
	SetUpdateFunc([this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2) {
		p2;
		p1->SetPosition({0, 0});
		p1->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});

		if(state_ == TransitionState::Closing) {
			p1->SetAlpha(progress);
		} else {
			p1->SetAlpha(1.0f - progress);
		}
	});
}

void TransitionControl::SetPresetSplit() {
	plate1_->Update(0.0f);
	plate2_->Update(0.0f);
	SetUpdateFunc([this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2) {
		if(!p2) return;
		float w		= static_cast<float>(kWindowWidth);
		float halfW = w * 0.5f;
		float h		= static_cast<float>(kWindowHeight);

		p1->SetScale({halfW, h});
		p2->SetScale({halfW, h});

		float offset = 0.0f;
		if(state_ == TransitionState::Closing) {
			// 左右から中央へ
			offset = halfW * (1.0f - progress);
			p1->SetPosition({-offset, 0});
			p2->SetPosition({halfW + offset, 0});
		} else {
			// 中央から左右へ
			offset = halfW * progress;
			p1->SetPosition({-offset, 0});
			p2->SetPosition({halfW + offset, 0});
		}
	});
}

void TransitionControl::SetPresetUpDownSlide() {
	// --- 調整用パラメータ ---
	const float kScale			= 1.7f;	   // 拡大率
	const float kTargetY	    = -328.0f; // 目標のY座標(一番上のプレートの終着点)
	const float kPlateSpacingY	= 200.0f;  // 各プレートの間隔

	// Closing(閉じる)時のアニメーション設定
	const float kCloseDuration	= 0.5f;	   // 各プレートが動き始めてから終わるまでの時間(0.0～1.0の割合)
	const float kCloseStagger	= 0.166f;  // 次のプレートが動き始めるまでの遅延

	// Opening(開く)時のアニメーション設定
	const float kOpenStartTime	= 0.5f;	   // 開き始めるタイミング
	const float kOpenDuration	= 0.25f;   // 各プレートの移動時間
	const float kOpenStagger	= 0.083f;  // 各プレートの開始のずれ

	// 波揺れ設定
	const float kWaveAmpX		= 20.0f;   // 横揺れの振幅
	const float kWaveFreq		= 4.0f;	   // 横揺れの速さ
	const float kMaxRot			= 0.05f;   // 最大回転角(ラジアン)
	const float kWavePhaseStep	= 0.5f;	   // プレートごとの位相のずれ
	// ------------------------

	plate1_->Update(0.0f);
	plate2_->Update(0.0f);
	plate3_->Update(0.0f);
	plate4_->Update(0.0f);
	if(plateAnim1_) plateAnim1_->Play("Wave");
	if(plateAnim2_) plateAnim2_->Play("Wave2");
	if(plateAnim3_) plateAnim3_->Play("Wave3");
	if(plateAnim4_) plateAnim4_->Play("Wave4");

	SetUpdateFunc([this, kScale, kTargetY, kPlateSpacingY, kCloseDuration, kCloseStagger, kOpenStartTime, kOpenDuration, kOpenStagger, kWaveAmpX, kWaveFreq, kMaxRot, kWavePhaseStep](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2) {
		if(!p1 || !p2 || !plate3_ || !plate4_) return;

		float w = static_cast<float>(kWindowWidth);
		float h = static_cast<float>(kWindowHeight);
		
		// スケールの一括設定
		const CalyxEngine::Vector2 scale = {w * kScale, h * kScale};
		p1->SetScale(scale);
		p2->SetScale(scale);
		plate3_->SetScale(scale);
		plate4_->SetScale(scale);

		// X座標のベース設定
		const float kPosXEven = -w * 0.2f;
		const float kPosXOdd  = -w * 0.4f;
		const float kRotPhaseOffset	= 0.8f;	   // 横揺れと回転の位相のずれ

		// 波揺れと回転の計算用ヘルパー
		auto getWave = [&](int index) {
			float phase = index * kWavePhaseStep;
			float offset = std::sin(timer_ * kWaveFreq + phase) * kWaveAmpX;
			// 回転の位相をずらす
			float rot	 = std::sin(timer_ * kWaveFreq + phase + kRotPhaseOffset) * kMaxRot;
			return std::make_pair(offset, rot);
		};

		auto [w1, r1] = getWave(0);
		auto [w2, r2] = getWave(1);
		auto [w3, r3] = getWave(2);
		auto [w4, r4] = getWave(3);

		// 回転の適用
		p1->SetRotation(r1);
		p2->SetRotation(r2);
		plate3_->SetRotation(r3);
		plate4_->SetRotation(r4);

		if(state_ == TransitionState::Closing) {
			// 各プレートのローカル進捗(0.0～1.0)を計算
			float p1_local = std::clamp((progress - kCloseStagger * 0) / kCloseDuration, 0.0f, 1.0f);
			float p2_local = std::clamp((progress - kCloseStagger * 1) / kCloseDuration, 0.0f, 1.0f);
			float p3_local = std::clamp((progress - kCloseStagger * 2) / kCloseDuration, 0.0f, 1.0f);
			float p4_local = std::clamp((progress - kCloseStagger * 3) / kCloseDuration, 0.0f, 1.0f);

			// 下から上へ移動 (横揺れw1～w4を加算)
			p1->SetPosition({kPosXEven + w1, h + (kTargetY - h) * p1_local});
			p2->SetPosition({kPosXOdd  + w2, h + (kTargetY - kPlateSpacingY * 1 - h) * p2_local});
			plate3_->SetPosition({kPosXEven + w3, h + (kTargetY - kPlateSpacingY * 2 - h) * p3_local});
			plate4_->SetPosition({kPosXOdd  + w4, h + (kTargetY - kPlateSpacingY * 3 - h) * p4_local});
		} else {
			// 各プレートのローカル進捗(0.0～1.0)を計算
			float p4_local = std::clamp((progress - kOpenStartTime - kOpenStagger * 0) / kOpenDuration, 0.0f, 1.0f);
			float p3_local = std::clamp((progress - kOpenStartTime - kOpenStagger * 1) / kOpenDuration, 0.0f, 1.0f);
			float p2_local = std::clamp((progress - kOpenStartTime - kOpenStagger * 2) / kOpenDuration, 0.0f, 1.0f);
			float p1_local = std::clamp((progress - kOpenStartTime - kOpenStagger * 3) / kOpenDuration, 0.0f, 1.0f);

			// 上から下へ移動して画面外へ (横揺れw1～w4を加算)
			plate4_->SetPosition({kPosXOdd  + w4, (kTargetY - kPlateSpacingY * 3) + (h - (kTargetY - kPlateSpacingY * 3)) * p4_local});
			plate3_->SetPosition({kPosXEven + w3, (kTargetY - kPlateSpacingY * 2) + (h - (kTargetY - kPlateSpacingY * 2)) * p3_local});
			p2->SetPosition({kPosXOdd  + w2, (kTargetY - kPlateSpacingY * 1) + (h - (kTargetY - kPlateSpacingY * 1)) * p2_local});
			p1->SetPosition({kPosXEven + w1, (kTargetY) + (h - (kTargetY)) * p1_local});
		}
	});
}

void TransitionControl::SetAutoPreset(SceneType now, SceneType next) {
	isDrawPlate2_ = false;
	isDrawPlate3_ = false;
	isDrawPlate4_ = false;
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->SetUvScale({1.0f, 1.0f});
	plate2_->SetUvScale({1.0f, 1.0f});
	plate1_->SetUvOffset({0.0f, 0.0f});
	plate2_->SetUvOffset({0.0f, 0.0f});
	plate1_->SetRotation(0.0f);
	plate2_->SetRotation(0.0f);
	// タイトルからセレクトへはスライド
	if(now == SceneType::TITLE && next == SceneType::SELECT) {
		isDrawPlate2_ = true;
		isDrawPlate3_ = true;
		isDrawPlate4_ = true;
		InitAnim();
		SetPresetUpDownSlide();
	}
	// テスト（ゲーム)からセレクトへは観音開き
	else if(now == SceneType::TEST && next == SceneType::SELECT) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// テスト（ゲーム)からタイトルへは観音開き
	else if(now == SceneType::TEST && next == SceneType::TITLE) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// ゲームオーバーやクリアへの遷移はフェード
	else if(next == SceneType::GAMEOVER || next == SceneType::CLEAR) {
		SetPresetFade();
	}
	// その他はデフォルトでフェード
	else {
		SetPresetFade();
	}
}

void TransitionControl::SetAutoPresetFromPrevious(SceneType prev, SceneType now) {
	isDrawPlate2_ = false;
	isDrawPlate3_ = false;
	isDrawPlate4_ = false;

	// 初期位置や初期サイズを設定しておかないと一瞬Init状態で描画される
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->SetUvScale({1.0f, 1.0f});
	plate2_->SetUvScale({1.0f, 1.0f});
	plate1_->SetUvOffset({0.0f, 0.0f});
	plate2_->SetUvOffset({0.0f, 0.0f});
	plate1_->SetRotation(0.0f);
	plate2_->SetRotation(0.0f);
	float w		= static_cast<float>(kWindowWidth);
	float halfW = w * 0.5f;
	float h		= static_cast<float>(kWindowHeight);
	//float halfH = h * 0.5f;
	// タイトルからセレクトに来た時はスライドで開ける
	if(prev == SceneType::TITLE && now == SceneType::SELECT) {
		isDrawPlate2_ = true;
		isDrawPlate3_ = true;
		isDrawPlate4_ = true;
		InitAnim();
		const float kPosXEven = w * 0.2f;
		const float kPosXOdd  = -w * 0.4f;
		float yD   = -328.0f;
		float disY = 200.0f;
		float xS = 1.7f;
		plate1_->SetScale({w * xS, h * xS});
		plate2_->SetScale({w * xS, h * xS});
		plate3_->SetScale({w * xS, h * xS});
		plate4_->SetScale({w * xS, h * xS});
		plate1_->SetPosition({kPosXEven, yD});
		plate2_->SetPosition({kPosXOdd, yD - disY});
		plate3_->SetPosition({kPosXEven, yD - disY * 2});
		plate4_->SetPosition({kPosXOdd, yD - disY * 3});
		SetPresetUpDownSlide();
	}
	// ゲームからセレクトに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::SELECT) {
		isDrawPlate2_ = true;
		plate1_->SetScale({halfW, h});
		plate2_->SetScale({halfW, h});

		float offset = 0.0f;
		offset = halfW;
		plate1_->SetPosition({0.0f, 0});
		plate2_->SetPosition({halfW, 0});
		SetPresetSplit();
	}
	// ゲームからタイトルに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::TITLE) {
		isDrawPlate2_ = true;
		plate1_->SetScale({halfW, h});
		plate2_->SetScale({halfW, h});

		float offset = 0.0f;
		offset		 = halfW;
		plate1_->SetPosition({0.0f, 0});
		plate2_->SetPosition({halfW, 0});
		SetPresetSplit();
	}
	// その他、フェードで開ける
	else {
		SetPresetFade();
	}
}

void TransitionControl::SetTexturePlate1(const std::string& texPath1) {
	plate1_->SetTexture(texPath1);
}

void TransitionControl::SetTexturePlate2(const std::string& texPath2) {
	plate2_->SetTexture(texPath2);
}

void TransitionControl::InitAnim() {
	plate3_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	plate3_->Initialize("Textures/Transition/wave.png");
	plate3_->SetPosition({0.0f, 720.0f});
	plate3_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
	plate3_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate3_->Update(0.0f);
	plate4_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	plate4_->Initialize("Textures/Transition/wave.png");
	plate4_->SetPosition({0.0f, 720.0f});
	plate4_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
	plate4_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate4_->Update(0.0f);
	
	plateAnim1_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim1_->Bind(plate1_.get());
	plateAnim2_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim2_->Bind(plate2_.get());
	plateAnim3_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim3_->Bind(plate3_.get());
	plateAnim4_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim4_->Bind(plate4_.get());

	auto asset		   = std::make_shared<CalyxEngine::SpriteAnimationAsset>();
	asset->division	   = {5, 4}; // 例: 4x4分割
	asset->texturePath = "Textures/Transition/wave.png";

	CalyxEngine::SpriteAnimationClip clip;
	clip.name		   = "Wave";
	clip.startFrame	   = 0;
	clip.frameCount	   = 20;
	clip.frameDuration = 0.175f;
	clip.loop		   = true;
	asset->clips.push_back(clip);

	CalyxEngine::SpriteAnimationClip clip2 = clip;
	clip2.name							   = "Wave2";
	clip2.frameDuration					   = 0.15f;
	asset->clips.push_back(clip2);
	CalyxEngine::SpriteAnimationClip clip3 = clip;
	clip3.name							   = "Wave3";
	clip3.frameDuration					   = 0.125f;
	asset->clips.push_back(clip3);
	CalyxEngine::SpriteAnimationClip clip4 = clip;
	clip4.name							   = "Wave4";
	clip4.frameDuration					   = 0.1f;
	asset->clips.push_back(clip4);

	plateAnim1_->SetAnimationAsset(asset);
	plateAnim2_->SetAnimationAsset(asset);
	plateAnim3_->SetAnimationAsset(asset);
	plateAnim4_->SetAnimationAsset(asset);
}
