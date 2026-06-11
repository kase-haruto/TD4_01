#include "GroundSpikeObject.h"

#include "Game\DemoShockwave\Shockwave.h"

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

	if(isBreak_ || isBuried_) return;
	if(auto* wave = dynamic_cast<Shockwave*>(otherObj)) {
		if(wave->IsStrong()) {
			isBreak_ = true;
		}
	}
	isBuried_ = true;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	EffectAPI::Stop(fxHandle_);
	fxHandle_ = {};
}

void GroundSpikeObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isSpike_  = false;
	isBreak_  = false;
	isBuried_ = false;

	worldTransform_.inheritScale = false;

	effectData_.Load("GroundSpikeEffect");
	dangerData_.Load("DangerEffect");

	AudioAPI::Load(spikeAudio_, "teeth");
}

void GroundSpikeObject::ObjectUpdate(float dt) {

	if(!dangerEffect_) {
		worldTransform_.Update();
		const auto position = worldTransform_.GetWorldPosition();
		fxHandle_ = EffectAPI::Play(dangerData_, {position.x, 1.5f, position.z});
		dangerEffect_ = true;
	}

	// 飛び出す時の処理
	if(isSpike_ && !isBuried_) {
		float y = worldTransform_.translation.y;
		y += param_.popUpSpeed * dt;
		worldTransform_.translation.y = y;
		if(param_.popUpHeight <= worldTransform_.GetWorldPosition().y) {
			isSpike_	= false;
			auto offset = CalyxEngine::Vector3(0.0f, 1.0f, 0.0f);
			EffectAPI::Play(effectData_, worldTransform_.GetWorldPosition() + offset);
			EffectAPI::Stop(fxHandle_);
			fxHandle_ = {};
			AudioAPI::Play(spikeAudio_, false, 0.5f);
		}
	}

	// 埋まる時の処理
	if (isBuried_) {
		if(worldTransform_.GetWorldPosition().y <= -1.4f) {
			//BaseGameObject::SetDrawEnable(false);
			isSpike_ = false;
			isBuried_ = false;
			if(isBreak_) {
				worldTransform_.scale.x = 1.2f;
				worldTransform_.scale.y = 0.75f;
				worldTransform_.scale.z = 1.2f;
			} else {
				worldTransform_.scale.x = 1.1f;
				worldTransform_.scale.z = 1.1f;
			}
		} else {
			worldTransform_.translation.y -= (param_.popUpSpeed * 0.5f) * dt;	
		}
	}

}
