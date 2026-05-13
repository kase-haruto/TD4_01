#include "DiceSocketObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DiceSocketObject)

DiceSocketObject::DiceSocketObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 2.5f;
}

const CalyxEngine::Vector3 DiceSocketObject::GetSocketPos() {
	CalyxEngine::Vector3 position = worldTransform_.translation;
	float scale = 0.5f;
	int index = 1 - clearCount_ + (diceSocketCount_ * 2);
	position.x += scale * static_cast<float>(index);
	return position;
}

void DiceSocketObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	BaseGameObject::SetColor({1.0f, 0.0f, 0.0f, 1.0f});
}

void DiceSocketObject::ObjectUpdate(float) {
}
