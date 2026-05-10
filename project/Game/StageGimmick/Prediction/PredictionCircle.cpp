#include "PredictionCircle.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include "Game\StageGimmick\Gimmicks\DroolRain\DroolRainObject.h"

REGISTER_SCENE_OBJECT(PredictionCircle)

PredictionCircle::PredictionCircle(
	const std::string& modelName, 
	std::optional<std::string> objectName): 
	BaseGameObject(modelName, objectName) {}

void PredictionCircle::Initialize() {

	worldTransform_.translation.y = 0.01f;
	worldTransform_.inheritScale = false;
	worldTransform_.inheritTranslate = false;
	BaseGameObject::Initialize();

	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}

	if(target_) {
		targetObjectY_ = target_->GetWorldPosition().y;
	}
}

void PredictionCircle::Update(float dt) {

	// 高さによってスケールを変化させる
	if(target_) {
		float scale			  = target_->GetWorldPosition().y / targetObjectY_;
		worldTransform_.scale = CalyxEngine::Vector3::One() * scale;
	}
	dt;
}

void PredictionCircle::SetTargetObject(DroolRainObject* target) {
	target_		   = target;
	targetObjectY_ = target_->GetWorldPosition().y;
}
