#include "Stage.h"
#include <Engine/Application/System/Environment.h>

void Stage::Initialize(float hp, float limitTime) {
	stageHP_ = hp;
	maxHP_	 = hp;
	timer_	 = limitTime;

	CalyxEngine::Vector2 centerPos = {static_cast<float>(kWindowWidth) * 0.5f, 60.0f};

	baseUI_ = std::make_unique<Sprite>("Textures/UI/Demon/demon_UI_solid.png");
	baseUI_->SetAnchorPoint({0.5f, 0.5f});
	baseUI_->SetSize(hpUISize_);
	baseUI_->SetPosition(centerPos);

	frameUI_ = std::make_unique<Sprite>("Textures/UI/Demon/demon_UI.png");
	frameUI_->SetAnchorPoint({0.5f, 0.5f});
	frameUI_->SetSize(hpUISize_);
	frameUI_->SetPosition(centerPos);

	hpUI_ = std::make_unique<Sprite>("Textures/white1x1.dds");
	hpUI_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
	hpUI_->SetAnchorPoint({0.0f, 0.5f});
	hpUI_->SetSize(barSize_);
	hpUI_->SetPosition({centerPos.x - barSize_.x * 0.5f + 55.0f, centerPos.y});

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers", ".png");
	CalyxEngine::Vector2 scorePos = {100.0f, 100.0f};
	numbersSprite_->Initialize(/*pos*/ scorePos,
							   /*digitSize*/ {32.0f, 32.0f});
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
	if(baseUI_) baseUI_->Update();
	if(frameUI_) frameUI_->Update();
	if(hpUI_) {
		hpUI_->Update();
	}
}

void Stage::Draw(SpriteRenderer* renderer) {
	if(numbersSprite_) {
		numbersSprite_->Draw(renderer);
	}
	if(baseUI_) renderer->Register(baseUI_.get());
	if(hpUI_) renderer->Register(hpUI_.get());
	if(frameUI_) renderer->Register(frameUI_.get());
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
	if(stageHP_ < 0.0f) {
		stageHP_ = 0.0f;
		isClear_ = true;
	}

	float rate = stageHP_ / maxHP_;
	hpUI_->SetSize({barSize_.x * rate, barSize_.y});

	// テクスチャの切り替え
	if (rate > 0.75f) {
		frameUI_->SetTexture("Textures/UI/Demon/demon_UI.png");
	} else if (rate > 0.5f) {
		frameUI_->SetTexture("Textures/UI/Demon/demon_UI01.png");
	} else if (rate > 0.25f) {
		frameUI_->SetTexture("Textures/UI/Demon/demon_UI02.png");
	} else {
		frameUI_->SetTexture("Textures/UI/Demon/demon_UI03.png");
	}
}
