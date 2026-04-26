#include "BreakableFloorEvent.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include "Engine/Foundation/Input/Input.h"

REGISTER_SCENE_OBJECT(BreakableFloorEvent)

BreakableFloorEvent::BreakableFloorEvent(const std::string& name) : StageGimmickEventBase(name) {}

void BreakableFloorEvent::SetTarget(const std::shared_ptr<BreakableFloorObject>& target) {
	targetObject_ = target;
}

void BreakableFloorEvent::OnCollisionEnter(Collider* other) {

	other;
	// ハンマー判定かどうか確認する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーの攻撃に当たったら床を壊す
	auto floor = targetObject_.lock();
	if(!floor) {
		return;
	}
	floor->Break();

	// イベントを無効化する
	
	isActive_ = false;
}

void BreakableFloorEvent::EventInitialize() {
	if(!targetObject_.expired()) {
		return;
	}

	std::string eventName = GetName();

	const std::string eventPrefix  = "BreakableFloorEvent";
	const std::string objectPrefix = "BreakableFloorObject";

	// イベント名が"BreakableFloorEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string targetName = objectPrefix + suffix;
	// シーンから対応するオブジェクトを探す
	targetObject_ =
		SceneContext::Current()->FindObjectByName<BreakableFloorObject>(targetName);
}

void BreakableFloorEvent::EventUpdate(float dt) {
	dt;
}
