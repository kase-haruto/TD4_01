#include "PitfallObject.h"

#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(PitfallObject)

PitfallObject::PitfallObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void PitfallObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
		if(auto* radius = dynamic_cast<BoxCollider*>(collider_.get())) {
			radius->SetSize(worldTransform_.scale);
		}
	}

	effectData_.Load("PitfallEffect");
}

void PitfallObject::ObjectUpdate(float) {
	if(collider_) {
		if(!collider_->GetOwner()) {
			collider_->SetOwner(this);
		}
	}
	if(!drawEffect_) {
		worldTransform_.Update();
		auto offset = CalyxEngine::Vector3(0.0f, -0.1f, 0.0f);
		EffectAPI::Play(effectData_, worldTransform_.GetWorldPosition() + offset);
		drawEffect_ = true;
	}

}