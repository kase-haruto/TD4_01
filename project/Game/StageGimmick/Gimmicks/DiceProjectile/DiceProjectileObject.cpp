#include "DiceProjectileObject.h"

#include <Game\StageGimmick\Gimmicks\DiceProjectile\DiceSocketObject.h>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DiceProjectileObject)

DiceProjectileObject::DiceProjectileObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 2.5f;
}

void DiceProjectileObject::OnCollisionEnter(Collider* other) {

	// ハンマー判定かどうか確認する
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}

	// 収納箱の座標を取得する
	targetPos_ = socket_->GetSocketPos();
	// 収納箱の数をプラスする
	socket_->AddDiceSocketCount();
	isPary_ = true;
}

void DiceProjectileObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
}

void DiceProjectileObject::ObjectUpdate(float dt) {

	// 飛んでいなければ更新を飛ばす
	if(!isFlying_) {
		return;
	}

	// パラメータ方向に飛ばす
	if(!isPary_) {
		CalyxEngine::Vector3 dire	  = param_.direction.Normalize();
		CalyxEngine::Vector3 velocity = dire * param_.speed * dt;
		worldTransform_.translation += velocity;
	} else {
		CalyxEngine::Vector3 dire	  = targetPos_ - worldTransform_.translation;
		CalyxEngine::Vector3 velocity = dire * param_.speed * dt;
		worldTransform_.translation += velocity;
	}

}
