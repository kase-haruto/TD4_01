#include "Stage.h"

void Stage::Initialize(float hp, float limitTime) {
	stageHP_ = hp;
	timer_	 = limitTime;
}

void Stage::Update(float dt) {
	timer_ -= dt;
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
}
