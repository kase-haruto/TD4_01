#pragma once
#include <Engine\Objects\2D\NumbersSprite\NumbersSprite.h>
#include <Engine/Renderer/Sprite/Sprite.h>

class Stage {
public:
	Stage() = default;
	~Stage() = default;

	void Initialize(float hp,float limitTime);

	void Update(float dt);

	void Draw(SpriteRenderer* renderer);

	void TakeDamage(float damage);

private:
	std::unique_ptr<Sprite>		   hpUI_;
	std::unique_ptr<NumbersSprite> numbersSprite_;
	float						   stageHP_ = 0.0f;
	float						   maxHP_	= 0.0f;
	float						   timer_	= 0.0f;

	CalyxEngine::Vector2 hpUISize_ = {500.0f, 20.0f};

};
