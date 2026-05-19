#include "BellProjectileTarget.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BellProjectileTarget)

BellProjectileTarget::BellProjectileTarget(const std::string& modelName, std::optional<std::string> objectName) 
	: StageGimmickObjectBase(modelName, objectName) {
}

const CalyxEngine::Vector3 BellProjectileTarget::GetTargetPos() const {
	return worldTransform_.translation;
}

void BellProjectileTarget::ObjectInitialize() {
	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_None);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	worldTransform_.scale = param_.scale;
	isRing_ = false;
}

void BellProjectileTarget::ObjectUpdate(float) {
	ChangeScale();
}

