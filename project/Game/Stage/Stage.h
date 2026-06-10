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

	bool IsGameOver() const { return isGameOver_; }
	bool IsClear() const { return isClear_; }

private:
	std::unique_ptr<Sprite> baseUI_;
	std::unique_ptr<Sprite> timeUI_;
	std::unique_ptr<Sprite> hpUI_;
	std::unique_ptr<Sprite> needleUI_;

	//std::unique_ptr<NumbersSprite> numbersSprite_;
	float						   stageHP_ = 0.0f;
	float						   maxHP_	= 0.0f;
	float						   timer_	= 0.0f;
	float						   limitTime_ = 0.0f;

	bool isGameOver_ = false;
	bool isClear_	 = false;

	CalyxEngine::Vector2 hpUISize_ = {240.0f, 240.0f};
	CalyxEngine::Vector2 barSize_   = {500.0f, 50.0f};

};
