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
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
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
	isFlying_ = false;
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(!isFlying_) {
		return;
	}
	auto player = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer");
	CalyxEngine::Vector3 velocity = player->GetWorldTransform().translation - worldTransform_.translation;

	if(!isHoming_){
		velocity = CalyxEngine::Vector3(0.0f, -0.5f, -1.0f);
	}

	velocity = velocity.Normalize() * speed_ * dt;
	worldTransform_.translation += velocity;
}
