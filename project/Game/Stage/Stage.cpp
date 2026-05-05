#include "Stage.h"

void Stage::Initialize(float hp, float limitTime) {
	stageHP_ = hp;
	timer_	 = limitTime;

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers", ".png");
	CalyxEngine::Vector2 scorePos = {100.0f, 100.0f};
	numbersSprite_->Initialize(/*pos*/ scorePos,
							   /*digitSize*/ {32.0f, 32.0f});
	numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Left);
}

void Stage::Update(float dt) {
	timer_ -= dt;
	if(numbersSprite_) {
		numbersSprite_->SetValue(int(timer_));
		numbersSprite_->Update();
	}
}

void Stage::Draw(SpriteRenderer* renderer) {
	if(numbersSprite_) {
		numbersSprite_->Draw(renderer);
	}
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
}
