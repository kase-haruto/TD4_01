#include "LuckyCatObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(LuckyCatObject)

LuckyCatObject::LuckyCatObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void LuckyCatObject::ObjectInitialize() {

	worldTransform_.translation.y += 1.5f;
	worldTransform_.translation.z += 5.0f;
	worldTransform_.scale = CalyxEngine::Vector3::One() * param_.luckyCatScale;
	worldTransform_.inheritScale = false;
}

void LuckyCatObject::ObjectUpdate(float) {
}
