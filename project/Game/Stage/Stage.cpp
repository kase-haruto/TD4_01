#include "Stage.h"
#include <Engine/Application/System/Environment.h>

void Stage::Initialize(float hp, float limitTime) {
	stageHP_   = hp;
	maxHP_	   = hp;
	timer_	   = limitTime;
	limitTime_ = limitTime;

	CalyxEngine::Vector2 centerPos = {120.0f, 120.0f};

	baseUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_Frame.png");
	baseUI_->SetAnchorPoint({0.5f, 0.5f});
	baseUI_->SetSize(hpUISize_);
	baseUI_->SetPosition(centerPos);

	timeUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_time.png");
	timeUI_->SetAnchorPoint({0.5f, 0.5f});
	timeUI_->SetSize(hpUISize_);
	timeUI_->SetPosition(centerPos);

	needleUI_ = std::make_unique<Sprite>("Textures/UI/Demon/enemy_HPUI_Needle.png");
	needleUI_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
	needleUI_->SetAnchorPoint({0.5f, 0.575f});
	needleUI_->SetSize(hpUISize_);
	needleUI_->SetPosition({centerPos.x, centerPos.y + 12.0f});

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers", ".png");
	CalyxEngine::Vector2 scorePos = {100.0f, 130.0f};
	numbersSprite_->Initialize(/*pos*/ scorePos,
							   /*digitSize*/ {20.0f, 36.0f});
	numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Left);
}

void Stage::Update(float dt) {
	if(timer_ > 0.0f) {
		timer_ -= dt;
	} else {
		timer_ = 0.0f;
		//isGameOver_ = true;
	}
	
	if(numbersSprite_) {
		numbersSprite_->SetValue(int(timer_));
		numbersSprite_->Update();
	}
	if(needleUI_ && limitTime_ > 0.0f) {
		// 経過時間に応じて針を回転させる(limitTimeで一回転)
		float elapsed	= limitTime_ - timer_;
		float twoPi		= 6.28318530718f;
		float rotation	= (elapsed / limitTime_) * twoPi;
		needleUI_->SetRotation(rotation);
	}

	if(baseUI_) baseUI_->Update();
	if(timeUI_) timeUI_->Update();
	if(needleUI_) needleUI_->Update();
}

void Stage::Draw(SpriteRenderer* renderer) {
	if(baseUI_) renderer->Register(baseUI_.get());
	if(timeUI_) renderer->Register(timeUI_.get());
	if(needleUI_) renderer->Register(needleUI_.get());
	if(numbersSprite_) {
		numbersSprite_->Draw(renderer);
	}
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
	if(stageHP_ < 0.0f) {
		stageHP_ = 0.0f;
		isClear_ = true;
	}

	float rate = stageHP_ / maxHP_;
	rate;
}
