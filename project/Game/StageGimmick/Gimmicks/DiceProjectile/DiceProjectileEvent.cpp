#include "DiceProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(DiceProjectileEvent);

DiceProjectileEvent::DiceProjectileEvent(const std::string& name) : StageGimmickEventBase(name) {}

void DiceProjectileEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// 全てのサイコロを飛ばす
	for(auto& target : targetObjects_) {
		if(target.lock()) {
			target.lock()->SetIsFlying(true);
		}
	}
}

void DiceProjectileEvent::EventInitialize() {

	targetObjects_.clear();
	targetObjects_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	std::string eventName = GetName();
	const std::string eventPrefix  = "DiceProjectileEvent";
	const std::string objectPrefix = "DiceProjectileObject(";
	const std::string socketPrefix = "DiceSocketObject";

	// イベント名が"DroolRainEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string objectName = eventName + "/" + objectPrefix;
	std::string socketName = socketPrefix + suffix;

	// シーンから対応するオブジェクトを探す
	auto object = SceneContext::Current()->FindObjectByName<DiceSocketObject>(socketName);
	if(object) {
		object->SetClearCount(eventData_.clearCount);
		object->Initialize();
		socket_ = object;

	// シーンから対応するオブジェクトが無ければ生成する
	} else {
		socket_ = SceneAPI::Instantiate<DiceSocketObject>("debugCube.obj", socketName);
		socket_.lock()->SetParent(shared_from_this());
		socket_.lock()->SetClearCount(eventData_.clearCount);
		socket_.lock()->Initialize();
	}

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::string indexedObjectName = objectName + std::to_string(i) + ")";
		auto target = SceneContext::Current()->FindObjectByName<DiceProjectileObject>(indexedObjectName);
		if(target) {
			targetObjects_[i] = target;
			targetObjects_[i].lock()->SetParam(eventParam_.param_);
			targetObjects_[i].lock()->SetSocket(socket_.lock().get());
			targetObjects_[i].lock()->Initialize();
			continue;
		}
		auto targetObject = SceneAPI::Instantiate<DiceProjectileObject>("dice.obj", indexedObjectName);
		if(targetObject) {
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->SetSocket(socket_.lock().get());
			targetObject->Initialize();
			targetObjects_[i] = (targetObject);
		}
	}
}

void DiceProjectileEvent::EventUpdate(float) {

	if(targetObjects_.empty()) {
		eventParam_.LoadParams();
		eventData_.SetEventName(GetName());
		eventData_.LoadParams();
		objectCount_ = eventData_.objectCount;
		EventInitialize();
	}

}

void DiceProjectileEvent::DerivativeGui() {

	ImGui::Text("DroolRainObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		++objectCount_;
		EventInitialize();
	}
	if(objectCount_ > 1) {
		ImGui::SameLine();
		if(ImGui::Button("-")) {
			--objectCount_;
			DeleteDroolObject();
		}
	}
	eventData_.ShowGui();
	eventParam_.ShowGui();
}

void DiceProjectileEvent::DeleteDroolObject() {

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}