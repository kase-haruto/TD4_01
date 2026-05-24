#pragma once
#include "Engine/Objects/2D/Object2d/SpriteObject2d.h"
#include <vector>
#include <memory>
#include <cstdint>

class SpriteRenderer;

class PlayerUI {
public:
	void Initialize(int32_t maxHP);
	void Update(float dt, int32_t currentHP);
	void Draw(SpriteRenderer* renderer);

private:
	std::unique_ptr<CalyxEngine::SpriteObject2d> baseSprite_;
	std::vector<std::unique_ptr<CalyxEngine::SpriteObject2d>> heartSprites_;

	// Config
	CalyxEngine::Vector2 basePos_	  = {150.0f, 650.0f}; // 画面左下付近
	CalyxEngine::Vector2 heartOffset_ = {-60.0f, 0.0f};   // 左詰めにするためのオフセット（baseの基準点からどれくらい離すか）
	
	// baseSprite_の中にheartを並べる
	// player_UIが枠で、その中にheartを並べる
	// player_UIのアンカーを左下にするか、中心にするか
};
