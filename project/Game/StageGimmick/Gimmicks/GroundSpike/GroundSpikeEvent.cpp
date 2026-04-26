#include "GroundSpikeEvent.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(GroundSpikeEvent)

GroundSpikeEvent::GroundSpikeEvent(const std::string& name) : StageGimmickEventBase(name) {}

void GroundSpikeEvent::SetTarget(const std::shared_ptr<GroundSpikeObject>& target) {
	targetObject_ = target;
}

void GroundSpikeEvent::OnCollisionEnter(Collider* other) {

	other;

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら歯を出す
	if(auto target = targetObject_.lock()) {
		target->Spike();
	}
}

void GroundSpikeEvent::EventInitialize() {

	if(!targetObject_.expired()) {
		return;
	}

	std::string eventName = GetName();

	const std::string eventPrefix  = "GroundSpikeEvent";
	const std::string objectPrefix = "GroundSpikeObject";

	// イベント名が"GroundSpikeEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string targetName = objectPrefix + suffix;
	// シーンから対応するオブジェクトを探す
	targetObject_ =
		SceneContext::Current()->FindObjectByName<GroundSpikeObject>(targetName);
}

void GroundSpikeEvent::EventUpdate(float dt) {
	dt;
}