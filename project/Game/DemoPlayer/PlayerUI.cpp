#include "PlayerUI.h"
#include "Engine/Renderer/Sprite/SpriteRenderer.h"

void PlayerUI::Initialize(int32_t maxHP) {
	baseSprite_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	baseSprite_->Initialize("Textures/UI/Player/player_UI.png");
	baseSprite_->SetAnchorPoint({0.0f, 1.0f}); // 左下
	baseSprite_->SetPosition({-30.0f, 720.0f}); // 画面左下
	baseSprite_->SetScale({640.0f, 180.0f});

	for (int i = 0; i < maxHP; ++i) {
		auto heart = std::make_unique<CalyxEngine::SpriteObject2d>();
		heart->Initialize("Textures/UI/Player/player_UI_heart.png");
		heart->SetAnchorPoint({0.0f, 1.0f});
		heart->SetScale({150.0f, 150.0f});
		heartSprites_.push_back(std::move(heart));
	}
}

void PlayerUI::Update(float dt, int32_t currentHP) {
	baseSprite_->Update(dt);

	// baseSpriteの左端からのオフセット
	float startX = 140.0f;
	float startY = 0.0f;
	
	CalyxEngine::Vector2 basePos = baseSprite_->GetPosition();

	for (int i = 0; i < heartSprites_.size(); ++i) {
		if (i < currentHP) {
			heartSprites_[i]->SetVisibility(true);
			// 左詰め
			heartSprites_[i]->SetPosition({basePos.x + startX + i * 75.0f, basePos.y + startY});
		} else {
			heartSprites_[i]->SetVisibility(false);
		}
		heartSprites_[i]->Update(dt);
	}
}

void PlayerUI::Draw(SpriteRenderer* renderer) {
	baseSprite_->Draw(renderer);
	for (auto& heart : heartSprites_) {
		heart->Draw(renderer);
	}
}
