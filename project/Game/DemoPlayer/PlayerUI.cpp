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

	hpSprite_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	hpSprite_->Initialize("Textures/UI/player/PlayerHP_01.png");
	hpSprite_->SetAnchorPoint({0.0f, 1.0f});	// 左下
	hpSprite_->SetPosition({-30.0f, 720.0f}); // 画面左下
	hpSprite_->SetScale({640.0f, 180.0f});

	damageAnim_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	damageAnim_->Bind(hpSprite_.get());

	auto asset		   = std::make_shared<CalyxEngine::SpriteAnimationAsset>();
	asset->division	   = {8, 1};
	asset->texturePath = "Textures/UI/player/PlayerHP_01.png";

	CalyxEngine::SpriteAnimationClip clip;
	clip.name		   = "damage";
	clip.startFrame	   = 0;
	clip.frameCount	   = 8;
	clip.frameDuration = 0.1f;
	clip.loop		   = false;
	asset->clips.push_back(clip);

	damageAnim_->SetAnimationAsset(asset);
}

void PlayerUI::Update(float dt, int32_t currentHP) {
	baseSprite_->Update(dt);

	if(!isOnce_) {
		isOnce_ = true;
		preHP_	= currentHP;
	}

	if(isDamageAnim_) {
		HPUpdate(dt);
	}
	hpSprite_->Update(dt);

	if(currentHP != preHP_) {
		isDamageAnim_ = true;
		damageAnim_->Play("damage", true);
		preHP_		  = currentHP;
	}

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
	/*baseSprite_->Draw(renderer);
	for (auto& heart : heartSprites_) {
		heart->Draw(renderer);
	}*/
	hpSprite_->Draw(renderer);
}

void PlayerUI::HPUpdate(float dt) {

	damageAnim_->Update(dt);

	if(isDamageAnim_ && damageAnim_->IsFinished()) {
		isDamageAnim_ = false;
		if(preHP_ == 4) {
			damageAnim_->GetAnimationAsset()->texturePath = "Textures/UI/player/PlayerHP_01.png";
		}else if(preHP_ == 3) {
			damageAnim_->GetAnimationAsset()->texturePath = "Textures/UI/player/PlayerHP_02.png";
			hpSprite_->SetTexture("Textures/UI/player/PlayerHP_02.png");
			damageAnim_->Reset();
			damageAnim_->Update(dt);
		} else if(preHP_ == 2) {
			damageAnim_->GetAnimationAsset()->texturePath = "Textures/UI/player/PlayerHP_03.png";
			hpSprite_->SetTexture("Textures/UI/player/PlayerHP_03.png");
			damageAnim_->Reset();
			damageAnim_->Update(dt);
		} else if(preHP_ == 1) {
			damageAnim_->GetAnimationAsset()->texturePath = "Textures/UI/player/PlayerHP_04.png";
			hpSprite_->SetTexture("Textures/UI/player/PlayerHP_04.png");
			damageAnim_->GetAnimationAsset()->division			  = {5, 1};
			damageAnim_->GetAnimationAsset()->clips[0].frameCount = 5;
			damageAnim_->Reset();
			damageAnim_->Update(dt);
		}
	}
}
