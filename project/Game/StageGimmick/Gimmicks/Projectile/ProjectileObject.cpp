#include "ProjectileObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include "Game\DemoPlayer\DemoPlayer.h"

REGISTER_SCENE_OBJECT(ProjectileObject)

ProjectileObject::ProjectileObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void ProjectileObject::OnCollisionEnter(Collider* other) {

	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack && other->GetType() != ColliderType::Type_Player) {
		return;
	}
	// コライダーとモデルを無効化
	isFlying_ = false;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}
}

void ProjectileObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isFlying_ = false;
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(!isFlying_) {
		if(isHoming_) {
			auto player	  = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer");
			velocity_ = player->GetWorldTransform().GetWorldPosition() - GetWorldTransform().GetWorldPosition();
		} else {
			velocity_ = CalyxEngine::Vector3(0.0f, -0.5f, -1.0f);
		}
		return;
	}

	velocity_ = velocity_.Normalize() * speed_ * dt;
	worldTransform_.translation += velocity_;
}
