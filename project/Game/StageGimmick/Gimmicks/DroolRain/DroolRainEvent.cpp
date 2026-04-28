#include "DroolRainEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(DroolRainEvent)

DroolRainEvent::DroolRainEvent(const std::string& name) : StageGimmickEventBase(name) {}


void DroolRainEvent::OnCollisionEnter(Collider* other) {

	other;

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;
}

void DroolRainEvent::OnCollisionExit(Collider* other) {

	other;
}

void DroolRainEvent::EventInitialize() {

	// プレイヤーが入ったらよだれを生成する
	const std::string objectName = "DroolRainObject";
	for(size_t i = 0; i < droolCount_; ++i) {
		if(droolCount_ <= droolObjects_.size()) { break; }
		auto object = SceneAPI::Instantiate<DroolRainObject>("debugCube.obj", objectName);
		object->SetParent(shared_from_this());
		droolObjects_.push_back(object);
	}
}

void DroolRainEvent::EventUpdate(float dt) {

	dt;
}
