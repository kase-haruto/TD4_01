#include "GeneralObject.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(GeneralObject)

GeneralObject::GeneralObject(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

}

void GeneralObject::OnCollisionEnter(Collider*) {
}

void GeneralObject::ColliderInitialize(const CalyxEngine::Vector3&) {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_Impediment);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
}

void GeneralObject::ObjectInitialize() {
}

void GeneralObject::ObjectUpdate(float) {
}
