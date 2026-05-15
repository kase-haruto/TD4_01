#include "TransitionControl.h"
#include <Engine/Application/System/Environment.h>
#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <algorithm>

TransitionControl::TransitionControl()	= default;
TransitionControl::~TransitionControl() = default;

void TransitionControl::Initialize(const std::string& texPath1, const std::string& texPath2) {
	plate1_ = std::make_unique<Sprite>(texPath1);
	plate1_->Initialize({0.0f, 0.0f}, {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
	plate1_->Update();

	if(!texPath2.empty()) {
		plate2_ = std::make_unique<Sprite>(texPath2);
		plate2_->Initialize({0.0f, 0.0f}, {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
		plate2_->Update();
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

	if(plate1_) plate1_->Update();
	if(plate2_) plate2_->Update();

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
			plate1_->SetSize({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate1_->Update();
		}
		if(plate2_) {
			plate2_->SetPosition({1280.0f, 0.0f});
			plate2_->SetAlpha(1.0f);
			plate2_->SetSize({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate2_->Update();
		}
	}
}

void TransitionControl::Draw(SpriteRenderer* renderer) {
	if(state_ == TransitionState::Idle) return;

	if(plate1_) renderer->Register(plate1_.get());
	if(plate2_ && isDrawPlate2_) renderer->Register(plate2_.get());
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
	SetUpdateFunc([toRight, this](float progress, Sprite* p1, Sprite* p2) {
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
	SetUpdateFunc([this](float progress, Sprite* p1, Sprite* p2) {
		p2;
		p1->SetPosition({0, 0});
		p1->SetSize({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});

		if(state_ == TransitionState::Closing) {
			p1->SetAlpha(progress);
		} else {
			p1->SetAlpha(1.0f - progress);
		}
	});
}

void TransitionControl::SetPresetSplit() {
	SetUpdateFunc([this](float progress, Sprite* p1, Sprite* p2) {
		if(!p2) return;

		float w		= static_cast<float>(kWindowWidth);
		float halfW = w * 0.5f;
		float h		= static_cast<float>(kWindowHeight);

		p1->SetSize({halfW, h});
		p2->SetSize({halfW, h});

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

void TransitionControl::SetAutoPreset(SceneType now, SceneType next) {
	isDrawPlate2_ = false;
	// タイトルからセレクトへはスライド
	if(now == SceneType::TITLE && next == SceneType::SELECT) {
		SetPresetSlide(false);
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
	// タイトルからセレクトに来た時はスライドで開ける
	if(prev == SceneType::TITLE && now == SceneType::SELECT) {
		SetPresetSlide(false);
	}
	// ゲームからセレクトに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::SELECT) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// ゲームからタイトルに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::TITLE) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// その他、フェードで開ける
	else {
		SetPresetFade();
	}
}
