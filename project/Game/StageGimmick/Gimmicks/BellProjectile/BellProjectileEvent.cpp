#include "BellProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BellProjectileEvent);

BellProjectileEvent::BellProjectileEvent(const std::string& name) : StageGimmickEventBase(name) {
}

void BellProjectileEvent::OnCollisionEnter(Collider* other) {
	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;
	eventParam_.doorParam_.scale = 100.0f;
	// プレイヤーがイベント内に入ったら飛ばす
	if(auto target = targetObject_.lock()) {
		target->SetIsFlying(true);
		target->SetIsFlying(true);
	}
}

void BellProjectileEvent::EventInitialize() {
	const std::string eventPrefix = "BellProjectileEvent";
	const std::string objectName = "BellProjectileObject";
	const std::string targetName = "BellProjectileTarget";
	const std::string doorName = "BellProjectileDoor";

	if(GetName().find(eventPrefix) == std::string::npos) {
		return;
	}

	// 鐘（ターゲット）の解決
	auto target = ResolveLinkedObject<BellProjectileTarget>(bellGuid_, targetName);
	if(!target) target = FindOwnedObjectByClassName<BellProjectileTarget>(targetName);
	if(target) {
		target->SetName(targetName);
		target->SetParam(eventParam_.targetParam_);
		target->Initialize();
		bell_ = target;
		bellGuid_ = target->GetGuid();
	} else {
		auto newTarget = SceneAPI::Instantiate<BellProjectileTarget>("cone.obj", targetName);
		newTarget->SetParent(shared_from_this(), false);
		newTarget->SetParam(eventParam_.targetParam_);
		newTarget->Initialize();
		bell_ = newTarget;
		bellGuid_ = newTarget->GetGuid();
	}

	// 撞木（オブジェクト）の解決
	auto obj = ResolveLinkedObject<BellProjectileObject>(targetObjectGuid_, objectName);
	if(!obj) obj = FindOwnedObjectByClassName<BellProjectileObject>(objectName);
	if(obj) {
		obj->SetName(objectName);
		obj->SetParam(eventParam_.param_);
		obj->SetTarget(bell_.lock().get());
		obj->Initialize();
		targetObject_ = obj;
		targetObjectGuid_ = obj->GetGuid();
	} else {
		auto newObj = SceneAPI::Instantiate<BellProjectileObject>("debugCube.obj", objectName);
		newObj->SetParent(shared_from_this(), false);
		newObj->SetParam(eventParam_.param_);
		newObj->SetTarget(bell_.lock().get());
		newObj->Initialize();
		targetObject_ = newObj;
		targetObjectGuid_ = newObj->GetGuid();
	}

	// 扉の解決
	auto door = ResolveLinkedObject<BellProjectileDoor>(doorGuid_, doorName);
	if(!door) door = FindOwnedObjectByClassName<BellProjectileDoor>(doorName);
	if(door) {
		door->SetName(doorName);
		door->SetParam(eventParam_.doorParam_);
		door->SetTarget(bell_.lock().get());
		door->Initialize();
		door_ = door;
		doorGuid_ = door->GetGuid();
	} else {
		auto newDoor = SceneAPI::Instantiate<BellProjectileDoor>("debugCube.obj", doorName);
		newDoor->SetParent(shared_from_this(), false);
		newDoor->SetParam(eventParam_.doorParam_);
		newDoor->SetTarget(bell_.lock().get());
		newDoor->Initialize();
		door_ = newDoor;
		doorGuid_ = newDoor->GetGuid();
	}
}

void BellProjectileEvent::EventUpdate(float) {
	if(targetObject_.expired()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		EventInitialize();
	}
}

void BellProjectileEvent::DerivativeGui() {
	eventParam_.ShowGui();
}

void BellProjectileEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	bellGuid_ = derived->value("bellGuid", Guid{});
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
	doorGuid_ = derived->value("doorGuid", Guid{});
	
	if(auto obj = targetObject_.lock()) {
		obj->SetParam(eventParam_.param_);
	}
	if(auto target = bell_.lock()) {
		target->SetParam(eventParam_.targetParam_);
	}
	if(auto door = door_.lock()) {
		door->SetParam(eventParam_.doorParam_);
	}
}

void BellProjectileEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	if(auto bell = bell_.lock()) {
		derived["bellGuid"] = bell->GetGuid();
	} else if(bellGuid_.isValid()) {
		derived["bellGuid"] = bellGuid_;
	}

	if(auto obj = targetObject_.lock()) {
		derived["targetObjectGuid"] = obj->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}

	if(auto door = door_.lock()) {
		derived["doorGuid"] = door->GetGuid();
	} else if(doorGuid_.isValid()) {
		derived["doorGuid"] = doorGuid_;
	}
}

void BellProjectileEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(bellGuid_, guidMap);
	RemapGuid(targetObjectGuid_, guidMap);
	RemapGuid(doorGuid_, guidMap);
}
