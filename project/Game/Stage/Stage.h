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
	float						   shakeTimer_ = 0.0f;
	float						   shakeTimeMax_ = 0.4f;
	float						   shakeStrong_	 = 8.0f;

	// 残り時間が少ない時の点滅(拡縮)演出
	float						   dangerThreshold_ = 10.0f; // この秒数以下で点滅開始
	float						   blinkTimer_		= 0.0f;	 // 点滅用の経過時間
	float						   blinkSpeed_		= 8.0f;	 // 点滅の速さ
	float						   blinkScale_		= 0.08f; // 拡縮の振れ幅(±8%)

	bool isGameOver_ = false;
	bool isClear_	 = false;

	CalyxEngine::Vector2 hpUISize_	  = {240.0f, 240.0f};
	CalyxEngine::Vector2 hpFillSize_  = {140.0f, 140.0f};
	CalyxEngine::Vector2 barSize_	  = {500.0f, 50.0f};
	CalyxEngine::Vector2 centerPos_	  = {120.0f, 120.0f};
	CalyxEngine::Vector2 centerDisPos_ = {120.0f, 133.0f};
};
