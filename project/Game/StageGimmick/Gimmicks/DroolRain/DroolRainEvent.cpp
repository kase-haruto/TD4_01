#include "DroolRainEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>

REGISTER_SCENE_OBJECT(DroolRainEvent)

DroolRainEvent::DroolRainEvent(const std::string& name) : StageGimmickEventBase(name) {}

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
			//lockedTarget->SetIsRaining(false);
		}
	}
}

void DroolRainEvent::SetTarget(const std::shared_ptr<DroolRainObject>& target) {
	targetObjects_.push_back(target);
}

void DroolRainEvent::EventInitialize() {

	targetObjects_.clear();
	predictionCircles_.clear();

	std::string eventName = GetName();
	const std::string eventPrefix  = "DroolRainEvent";
	const std::string objectPrefix = "DroolRainObject";
	const std::string predictionCirclePrefix = "PredictionCircle";

	// イベント名が"DroolRainEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string objectName = objectPrefix;
	std::string predictionCircleName = predictionCirclePrefix;
	auto childTargets = FindDirectChildrenOfType<DroolRainObject>(*this);
	if(!childTargets.empty()) {
		objectCount_		   = static_cast<uint32_t>(childTargets.size());
		eventData_.objectCount = objectCount_;
		targetObjects_.resize(objectCount_);
		predictionCircles_.resize(objectCount_);

		for(size_t i = 0; i < childTargets.size(); ++i) {
			targetObjects_[i] = childTargets[i];
			childTargets[i]->SetParam(eventParam_.param_);
			childTargets[i]->Initialize();

			auto childPredictionCircles = FindDirectChildOfType<PredictionCircle>(*targetObjects_[i].lock().get());
			if(!childPredictionCircles) {
				predictionCircles_[i] = childPredictionCircles;
				childPredictionCircles->SetParent(childTargets[i]);
				childPredictionCircles->Initialize();
			}
		}
		return;
	}

	targetObjects_.resize(objectCount_);
	predictionCircles_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", objectName);
		if(targetObject) {
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->Initialize();
			targetObjects_[i] = targetObject;
		}
		auto predictionCircle = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCircleName);
		if(predictionCircle && targetObjects_[i].lock()) {
			predictionCircle->SetParent(targetObjects_[i].lock());
			predictionCircle->Initialize();
			predictionCircles_[i] = predictionCircle;
		}
	}
}

void DroolRainEvent::EventUpdate(float) {

	if(targetObjects_.empty()) {
		eventParam_.LoadParams();
		eventData_.SetEventName(GetName());
		eventData_.LoadParams();
		objectCount_ = eventData_.objectCount;
		EventInitialize();
	} else {

		uint32_t index = 0;
		for(const auto& target : targetObjects_) {
			if(target.lock() && predictionCircles_[index].lock()) {
				CalyxEngine::Vector3 targetPos = target.lock()->GetWorldPosition();
				targetPos.y					   = 0.01f;
				predictionCircles_[index].lock()->SetTranslate(targetPos);
			}
			index++;
		}
	}
}

void DroolRainEvent::DerivativeGui() {

	ImGui::Text("DroolRainObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		AddDroolObject();
	}
	if(objectCount_ > 1) {
		ImGui::SameLine();
		if(ImGui::Button("-")) {
			DeleteDroolObject();
		}
	}
	eventData_.ShowGui();
	eventParam_.ShowGui();
}

void DroolRainEvent::AddDroolObject() {

	const std::string objectName			 = "DroolRainObject";
	const std::string eventPrefix			 = "DroolRainEvent";
	const std::string predictionCirclePrefix = "PredictionCircle";

	std::string predictionCircleName = predictionCirclePrefix;
	if(GetName().find(eventPrefix) == 0) {
		predictionCircleName += GetName().substr(eventPrefix.size());
	}

	auto targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", objectName);
	if(!targetObject) return;

	targetObject->SetParent(shared_from_this());
	targetObject->SetParam(eventParam_.param_);
	targetObject->Initialize();

	auto predictionCircle = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCircleName);
	if(predictionCircle) {
		predictionCircle->SetParent(targetObject);
		predictionCircle->Initialize();
	}

	targetObjects_.push_back(targetObject);
	predictionCircles_.push_back(predictionCircle);
	++objectCount_;
	eventData_.objectCount = objectCount_;
}

void DroolRainEvent::DeleteDroolObject() {

	--objectCount_;
	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			ctx->RemoveObject(predictionCircles_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
			predictionCircles_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}
