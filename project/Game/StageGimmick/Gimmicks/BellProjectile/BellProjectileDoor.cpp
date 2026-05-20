#include "BellProjectileDoor.h"
#include "BellProjectileTarget.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BellProjectileDoor)

BellProjectileDoor::BellProjectileDoor(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BellProjectileDoor::ObjectInitialize() {
	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_Impediment);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	worldTransform_.scale = param_.scale;
	startPos_ = worldTransform_.translation;
	isOpened_ = false;
	openRate_ = 0.0f;
}

void BellProjectileDoor::ObjectUpdate(float dt) {
	if (isOpened_) return;

	if (target_ && target_->GetIsRing()) {
		openRate_ += param_.speed * dt;
		if (openRate_ >= 1.0f) {
			openRate_ = 1.0f;
			isOpened_ = true;
			if (collider_) {
				collider_->SetCollisionEnabled(false);
			}
		}

		// 上に開く例（必要に応じて調整）
		worldTransform_.translation.y = startPos_.y + (openRate_ * 10.0f);
	}
}
