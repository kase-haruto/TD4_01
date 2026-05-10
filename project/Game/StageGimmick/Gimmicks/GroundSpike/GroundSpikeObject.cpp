#include "GroundSpikeObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(GroundSpikeObject)


GroundSpikeObject::GroundSpikeObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
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
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isSpike_ = false;
}

void GroundSpikeObject::ObjectUpdate(float dt) {
	dt;

	// 飛び出す時の処理
	if(isSpike_ && time_ != spikeTime_) {
		// 時間を進める
		time_ += dt;
		time_ = std::clamp(time_, 0.0f, spikeTime_);
		float t	   = time_ / spikeTime_;
		float ease = CalyxEngine::ApplyEase(CalyxEngine::EaseType::EaseOutBack, t);

		// 目標の座標に向かって補完する
		float aimY = std::lerp(worldTransform_.translation.y, spikeAimY_, ease);
		worldTransform_.translation.y = aimY;
	}
}
