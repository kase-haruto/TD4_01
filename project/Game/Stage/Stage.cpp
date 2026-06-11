#include "Stage.h"
#include <Engine/Application/System/Environment.h>
#include <cmath>

void Stage::Initialize(float hp, float limitTime) {
	stageHP_   = hp;
	maxHP_	   = hp;
	timer_	   = limitTime;
	limitTime_ = limitTime;

	baseUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_Frame.png");
	baseUI_->SetAnchorPoint({0.5f, 0.5f});
	baseUI_->SetSize(hpUISize_);
	baseUI_->SetPosition(centerPos_);

	timeUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_time.png");
	timeUI_->SetAnchorPoint({0.5f, 0.5f});
	timeUI_->SetSize(hpUISize_);
	timeUI_->SetPosition(centerPos_);

	hpUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_solid.png");
	hpUI_->SetAnchorPoint({0.5f, 0.5f});
	hpUI_->SetSize(hpFillSize_);
	hpUI_->SetPosition(centerDisPos_);
	hpUI_->SetFillMethod(2);
	hpUI_->SetFillOrigin(0.0f, 0.0f);
	hpUI_->SetFillAmount(1.0f);

	needleUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_Needle.png");
	needleUI_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
	needleUI_->SetAnchorPoint({0.5f, 0.575f});
	needleUI_->SetSize(hpUISize_);
	needleUI_->SetPosition(centerDisPos_);

	//numbersSprite_ = std::make_unique<NumbersSprite>(
	//	"Textures/Numbers", ".png");
	//CalyxEngine::Vector2 scorePos = {100.0f, 130.0f};
	//numbersSprite_->Initialize(/*pos*/ scorePos,
	//						   /*digitSize*/ {20.0f, 36.0f});
	//numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Left);
}

void Stage::Update(float dt) {
	if(timer_ > 0.0f) {
		timer_ -= dt;
	} else {
		timer_ = 0.0f;
		isGameOver_ = true;
	}
	
	/*if(numbersSprite_) {
		numbersSprite_->SetValue(int(timer_));
		numbersSprite_->Update();
	}*/
	if(needleUI_ && limitTime_ > 0.0f) {
		// 経過時間に応じて針を回転させる(limitTimeで一回転)
		float elapsed	= limitTime_ - timer_;
		float twoPi		= 6.28318530718f;
		float rotation	= (elapsed / limitTime_) * twoPi;
		needleUI_->SetRotation(rotation);
	}
	if(shakeTimer_ > 0.0f) {
		shakeTimer_ -= dt;

		CalyxEngine::Vector2 offset = {0.0f, 0.0f};
		if(shakeTimer_ > 0.0f) {
			float strength = shakeStrong_ * (shakeTimeMax_ > 0.0f ? shakeTimer_ / shakeTimeMax_ : 1.0f);
			offset = {Random::Generate<float>(-strength, strength), Random::Generate<float>(-strength, strength)};
		} else {
			shakeTimer_ = 0.0f;
		}

		baseUI_->SetPosition(centerPos_ + offset);
		timeUI_->SetPosition(centerPos_ + offset);
		hpUI_->SetPosition(centerDisPos_ + offset);
		needleUI_->SetPosition(centerDisPos_ + offset);
	}

	// 残り時間が少ない時は危機感を出すため点滅(拡縮)させる
	if(timer_ <= dangerThreshold_ && timer_ > 0.0f) {
		blinkTimer_ += dt;
		float s = std::sinf(blinkTimer_ * blinkSpeed_); // -1.0 ~ 1.0
		float scale = 1.0f + blinkScale_ * s;			// 1.0 ± blinkScale_
		float alpha = 0.6f + 0.4f * (0.5f * (s + 1.0f)); // 0.6 ~ 1.0

		if(baseUI_) { baseUI_->SetSize(hpUISize_ * scale); baseUI_->SetAlpha(alpha); }
		if(timeUI_) { timeUI_->SetSize(hpUISize_ * scale); timeUI_->SetAlpha(alpha); }
		if(hpUI_) { hpUI_->SetSize(hpFillSize_ * scale); hpUI_->SetAlpha(alpha); }
		if(needleUI_) { needleUI_->SetSize(hpUISize_ * scale); needleUI_->SetAlpha(alpha); }
	} else if(blinkTimer_ != 0.0f) {
		// 点滅終了時に元のサイズ・不透明度へ戻す
		blinkTimer_ = 0.0f;
		if(baseUI_) { baseUI_->SetSize(hpUISize_); baseUI_->SetAlpha(1.0f); }
		if(timeUI_) { timeUI_->SetSize(hpUISize_); timeUI_->SetAlpha(1.0f); }
		if(hpUI_) { hpUI_->SetSize(hpFillSize_); hpUI_->SetAlpha(1.0f); }
		if(needleUI_) { needleUI_->SetSize(hpUISize_); needleUI_->SetAlpha(1.0f); }
	}

	if(baseUI_) baseUI_->Update();
	if(timeUI_) timeUI_->Update();
	if(hpUI_) hpUI_->Update();
	if(needleUI_) needleUI_->Update();
}

void Stage::Draw(SpriteRenderer* renderer) {
	if(baseUI_) renderer->Register(baseUI_.get());
	if(hpUI_) renderer->Register(hpUI_.get());
	if(timeUI_) renderer->Register(timeUI_.get());
	if(needleUI_) renderer->Register(needleUI_.get());
	/*if(numbersSprite_) {
		numbersSprite_->Draw(renderer);
	}*/
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
	shakeTimer_ = shakeTimeMax_;
	if(stageHP_ < 0.0f) {
		stageHP_ = 0.0f;
		isClear_ = true;
	}

	float rate = stageHP_ / maxHP_;
	hpUI_->SetFillAmount(rate);
}
