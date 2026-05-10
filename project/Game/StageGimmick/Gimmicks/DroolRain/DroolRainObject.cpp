#include "DroolRainObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(DroolRainObject)

DroolRainObject::DroolRainObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void DroolRainObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
}

void DroolRainObject::ObjectUpdate(float dt) {

	//if(!isRaining_) {
	//	return;
	//}

	if(isOnceSet_) {
		offsetY_ = worldTransform_.translation.y;
		isOnceSet_ = false;
		return;
	}

	float y = worldTransform_.translation.y;
	y -= 5.0f * dt;
	worldTransform_.translation.y = y;

	if(worldTransform_.translation.y < -5.0f) {
		worldTransform_.translation.y = offsetY_;
	}
}
