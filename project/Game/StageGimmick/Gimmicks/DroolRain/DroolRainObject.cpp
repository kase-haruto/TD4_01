#include "DroolRainObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(DroolRainObject)

DroolRainObject::DroolRainObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void DroolRainObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
}

void DroolRainObject::ObjectUpdate(float dt) {

	float y = worldTransform_.translation.y;
	y -= 2.0f * dt;
	worldTransform_.translation.y = y;
}
