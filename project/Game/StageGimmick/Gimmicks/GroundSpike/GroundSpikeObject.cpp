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

void GroundSpikeObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
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
