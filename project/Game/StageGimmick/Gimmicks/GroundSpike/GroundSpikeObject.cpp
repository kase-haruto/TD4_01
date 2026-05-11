#include "GroundSpikeObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(GroundSpikeObject)


GroundSpikeObject::GroundSpikeObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) 
{
	worldTransform_.translation.y -= 5.0f;
}

void GroundSpikeObject::Spike() {
	isSpike_ = true;
}

void GroundSpikeObject::OnCollisionEnter(Collider* other) {
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}

	if(!isSpike_ || isBreak_) return;
	isBreak_ = true;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}
}

void GroundSpikeObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isSpike_ = false;

	worldTransform_.inheritScale = false;
}

void GroundSpikeObject::ObjectUpdate(float dt) {

	// 飛び出す時の処理
	if(isSpike_ && param_.popUpHeight > worldTransform_.GetWorldPosition().y) {
		float y = worldTransform_.translation.y;
		y += param_.popUpSpeed * dt;
		worldTransform_.translation.y = y;
	}

}
