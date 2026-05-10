#include "PredictionCircle.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(PredictionCircle)

PredictionCircle::PredictionCircle(
	const std::string& modelName, 
	std::optional<std::string> objectName): 
	BaseGameObject(modelName, objectName) {}

void PredictionCircle::Initialize() {

	worldTransform_.translation.y = 0.01f;
	BaseGameObject::Initialize();

	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void PredictionCircle::Update(float dt) {

	dt;
}
