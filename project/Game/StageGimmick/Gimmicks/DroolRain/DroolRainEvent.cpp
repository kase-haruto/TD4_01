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

	eventParam_.LoadParams();
	objectCount_ = 5;
	targetObjects_.resize(objectCount_);
	predictionCircles_.resize(objectCount_);

	std::string eventName = GetName();

	const std::string eventPrefix  = "DroolRainEvent";
	const std::string objectPrefix = "DroolRainObject(";
	const std::string predictionCirclePrefix = "PredictionCircle(";

	// イベント名が"DroolRainEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string objectName = eventName + "/" + objectPrefix;
	std::string predictionCircleName = eventName + "/" + predictionCirclePrefix;

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::string indexedObjectName = objectName + std::to_string(i) + ")";
		auto object = SceneContext::Current()->FindObjectByName<DroolRainObject>(indexedObjectName);
		if(object) {
			targetObjects_[i] = object;
			targetObjects_[i].lock()->SetParam(eventParam_.param_);
			continue;
		}
		auto targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", indexedObjectName);
		if(targetObject) {
			targetObject->SetParent(shared_from_this());
			targetObject->Initialize();
			targetObject->SetParam(eventParam_.param_);
			targetObject->GetWorldTransform().translation.y += 2.5f;
			targetObject->GetWorldTransform().inheritScale = false;
			targetObjects_[i] = (targetObject);
		}
	}

	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::string indexedPredictionCircleName = predictionCircleName + std::to_string(i) + ")";
		auto object = SceneContext::Current()->FindObjectByName<PredictionCircle>(indexedPredictionCircleName);
		if(object) {
			predictionCircles_[i] = object;
			continue;
		}
		auto targetObject = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", indexedPredictionCircleName);
		if(targetObject) {
			targetObject->Initialize();
			targetObject->SetParent(targetObjects_[i].lock());
			targetObject->GetWorldTransform().inheritScale = false;
			targetObject->GetWorldTransform().inheritTranslate = false;
			predictionCircles_[i] = (targetObject);
		}
	}
}

void DroolRainEvent::EventUpdate(float dt) {

	if(targetObjects_.empty()) {
		EventInitialize();
	} else {

		uint32_t index = 0;
		for(const auto& target : targetObjects_) {
			CalyxEngine::Vector3 targetPos = target.lock()->GetWorldPosition();
			targetPos.y = 0.01f;
			predictionCircles_[index].lock()->SetTranslate(targetPos);
			index++;
		}
	}

	dt;
}

void DroolRainEvent::DerivativeGui() {
	eventParam_.ShowGui();
}
