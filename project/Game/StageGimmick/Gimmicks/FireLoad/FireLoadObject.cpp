#include "FireLoadObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(FireLoadObject)

FireLoadObject::FireLoadObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void FireLoadObject::OnCollisionEnter(Collider* other) {
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}
	// コライダーとモデルを無効化
	isBurn_ = false;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}
}

void FireLoadObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	defaultScale_ = worldTransform_.scale;
}

void FireLoadObject::ObjectUpdate(float dt) {

	if(!isBurn_) return;
	float t = dt;
	t += dt;

}
