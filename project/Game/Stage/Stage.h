#pragma once
#include <Engine\Objects\2D\NumbersSprite\NumbersSprite.h>

class Stage {
public:
	Stage() = default;
	~Stage() = default;

	void Initialize(float hp,float limitTime);

	void Update(float dt);

	void Draw(SpriteRenderer* renderer);

	void TakeDamage(float damage);

private:

private:

	std::unique_ptr<NumbersSprite> numbersSprite_;
	float stageHP_ = 0.0f;
	float timer_   = 0.0f;

};
