#include "BellProjectileDoor.h"
#include "BellProjectileTarget.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Foundation/Math/MathUtil.h>

REGISTER_SCENE_OBJECT(BellProjectileDoor)

BellProjectileDoor::BellProjectileDoor(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BellProjectileDoor::SetLR(int lr) {
	lr_ = lr;
}

void BellProjectileDoor::ObjectInitialize() {
	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_Impediment);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
		if(auto* radius = dynamic_cast<BoxCollider*>(collider_.get())) {
			radius->SetSize({9.0f, 11.5f, 1.0f});
		}
	}

	worldTransform_.scale = param_.scale;
	worldTransform_.rotationSource = RotationSource::Euler;
	if (lr_ == 1) {
		//worldTransform_.eulerRotation.y = CalyxEngine::ToRadians(180.0f);
		if(collider_) {
			collider_->SetOffset({-4.5f, 5.75f, 0.0f});
		}
	} else {
		worldTransform_.eulerRotation.y = 0.0f;
		if(collider_) {
			collider_->SetOffset({4.5f, 5.75f, 0.0f});
		}
	}

	startPos_ = worldTransform_.translation;
	isOpenRequested_ = false;
	isOpened_ = false;
	openRate_ = 0.0f;
}

void BellProjectileDoor::ObjectUpdate(float dt) {
	if (isOpened_) return;

	if(isOpenRequested_ || (target_ && target_->GetIsRing())) {
		openRate_ += param_.speed * dt;
		if (openRate_ >= 1.0f) {
			openRate_ = 1.0f;
			isOpened_ = true;
			if (collider_) {
				collider_->SetCollisionEnabled(false);
			}
		}

		float angle = openRate_ * CalyxEngine::ToRadians(90.0f);
		if (lr_ == 0) {
			worldTransform_.eulerRotation.y = -angle;
		} else {
			worldTransform_.eulerRotation.y = angle;
		}
	}
}
