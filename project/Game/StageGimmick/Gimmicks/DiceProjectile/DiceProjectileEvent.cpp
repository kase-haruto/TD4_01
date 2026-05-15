#include "DiceProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>

REGISTER_SCENE_OBJECT(DiceProjectileEvent);

DiceProjectileEvent::DiceProjectileEvent(const std::string& name) : StageGimmickEventBase(name) {}

namespace {
	template <class TObject>
	std::shared_ptr<TObject> FindDirectChildOfType(const SceneObject& parent) {
		for(const auto& child : parent.GetChildren()) {
			if(auto casted = std::dynamic_pointer_cast<TObject>(child)) {
				return casted;
			}
		}
		return nullptr;
	}

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindDirectChildrenOfType(const SceneObject& parent) {
		std::vector<std::shared_ptr<TObject>> result;
		for(const auto& child : parent.GetChildren()) {
			if(auto casted = std::dynamic_pointer_cast<TObject>(child)) {
				result.push_back(std::move(casted));
			}
		}

		std::sort(result.begin(), result.end(),
				  [](const std::shared_ptr<TObject>& lhs,
					 const std::shared_ptr<TObject>& rhs) {
					  if(lhs->GetName() != rhs->GetName()) return lhs->GetName() < rhs->GetName();
					  return lhs->GetGuid().ToString() < rhs->GetGuid().ToString();
				  });
		return result;
	}
} // namespace

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
	std::string objectName = objectPrefix;
	std::string socketName = socketPrefix + suffix;

	// Prefab 配置済みの子があれば、名前ではなく親子関係から拾う
	auto object = FindDirectChildOfType<DiceSocketObject>(*this);
	if(!object) {
		object = SceneContext::Current()->FindObjectByName<DiceSocketObject>(socketName);
	}
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

	auto childTargets = FindDirectChildrenOfType<DiceProjectileObject>(*this);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
		eventData_.objectCount = objectCount_;
		targetObjects_.resize(objectCount_);

		for(size_t i = 0; i < childTargets.size(); ++i) {
			targetObjects_[i] = childTargets[i];
			childTargets[i]->SetParam(eventParam_.param_);
			childTargets[i]->SetSocket(socket_.lock().get());
			childTargets[i]->Initialize();
		}
		return;
	}

	targetObjects_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

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
