#include "DroolRainEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(DroolRainEvent)

DroolRainEvent::DroolRainEvent(const std::string& name) : StageGimmickEventBase(name) {}

void DroolRainEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら降らせる
	for(auto& target : targetObjects_) {
		if(auto lockedTarget = target.lock()) {
			lockedTarget->SetIsRaining(true);
		}
	}
}

void DroolRainEvent::OnCollisionExit(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内から出たら降らせるのをやめる
	for(auto& target : targetObjects_) {
		if(auto lockedTarget = target.lock()) {
			lockedTarget->SetIsRaining(false);
		}
	}
}

void DroolRainEvent::SetTarget(const std::shared_ptr<DroolRainObject>& target) {
	targetObjects_.push_back(target);
}

void DroolRainEvent::EventInitialize() {

	objectCount_ = 5;

	std::string eventName = GetName();

	const std::string eventPrefix  = "DroolRainEvent";
	const std::string objectPrefix = "DroolRainObject(";

	// イベント名が"DroolRainEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string targetName = eventName + "/" + objectPrefix;
	// シーンから対応するオブジェクトを探す
	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::string indexedTargetName = targetName + std::to_string(i) + ")";
		auto targetObject = SceneContext::Current()->FindObjectByName<DroolRainObject>(indexedTargetName);
		if(targetObject) {
			targetObjects_.push_back(targetObject);
		}
	}
}

void DroolRainEvent::EventUpdate(float dt) {

	dt;
}
