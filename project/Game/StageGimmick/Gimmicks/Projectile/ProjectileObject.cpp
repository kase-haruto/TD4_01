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

void ProjectileObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Box);
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(!isFlying_) {
		return;
	}
	auto player = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer");
	CalyxEngine::Vector3 velocity = player->GetWorldTransform().translation - worldTransform_.translation;

	velocity = velocity.Normalize() * speed_ * dt;
	worldTransform_.translation += velocity;
}
