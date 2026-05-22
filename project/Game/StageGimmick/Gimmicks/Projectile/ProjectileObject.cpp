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
	bool isPlayerAttack = other->GetType() == ColliderType::Type_PlayerAttack;
	bool isPlayer = other->GetType() == ColliderType::Type_Player;
	if(otherObj && !isPlayerAttack && !isPlayer) {
		return;
	}
	// コライダーとモデルを無効化
	isFlying_ = false;
	isHit_		= isPlayer;
	isParry_	= isPlayerAttack;
	targetTime_ = 0.0f;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void ProjectileObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isFlying_ = false;
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(!isFlying_) {
		if(targetTime_< 1.0f && isHit_) {
			targetTime_ += dt;
			if(targetTime_ >= 1.0f) {
				SetDrawEnable(false);
				velocity_ = CalyxEngine::Vector3::Zero();
			}
		}
	} else {
		if(param_.targetTime > targetTime_) {
			targetTime_ += dt;
			auto player = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer");
			velocity_	= player->GetWorldTransform().GetWorldPosition() - GetWorldTransform().GetWorldPosition();
		}
	}

	velocity_ = velocity_.Normalize() * param_.speed * dt;
	worldTransform_.translation += velocity_;
}
