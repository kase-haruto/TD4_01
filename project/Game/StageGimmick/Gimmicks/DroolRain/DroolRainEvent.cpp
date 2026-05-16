#include "DroolRainEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

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
			//lockedTarget->SetIsRaining(false);
		}
	}
}

void DroolRainEvent::SetTarget(const std::shared_ptr<DroolRainObject>& target) {
	targetObjects_.push_back(target);
	if(target) targetObjectGuids_.push_back(target->GetGuid());
}

void DroolRainEvent::EventInitialize() {

	targetObjects_.clear();
	predictionCircles_.clear();
	const std::string eventPrefix  = "DroolRainEvent";
	const std::string objectName = "DroolRainObject";
	const std::string predictionCircleName = "PredictionCircle";

	if(GetName() != eventPrefix) {
		return;
	}

	auto childTargets = ResolveLinkedObjects<DroolRainObject>(targetObjectGuids_, objectName);
	if(childTargets.empty()) childTargets = FindOwnedObjectsByClassName<DroolRainObject>(objectName);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
	}

	targetObjects_.resize(objectCount_);
	predictionCircles_.resize(objectCount_);
	targetObjectGuids_.resize(objectCount_);
	predictionCircleGuids_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::shared_ptr<DroolRainObject> targetObject =
			i < childTargets.size() ? childTargets[i] : nullptr;
		if(!targetObject) {
			targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", objectName);
		}
		if(targetObject) {
			targetObject->SetName(objectName);
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->Initialize();
			targetObjects_[i] = (targetObject);
			targetObjectGuids_[i] = targetObject->GetGuid();
		}
	}

	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto targetObject = ResolveLinkedObject<PredictionCircle>(predictionCircleGuids_[i], predictionCircleName);
		if(!targetObject) {
			auto target = targetObjects_[i].lock();
			auto childPredictionCircles = target
				? FindChildrenByClassName<PredictionCircle>(*target, predictionCircleName)
				: std::vector<std::shared_ptr<PredictionCircle>>{};
			targetObject = childPredictionCircles.empty() ? nullptr : childPredictionCircles.front();
		}
		if(!targetObject) {
			targetObject = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCircleName);
		}
		if(targetObject) {
			targetObject->SetName(predictionCircleName);
			targetObject->SetParent(targetObjects_[i].lock());
			targetObject->Initialize();
			predictionCircles_[i] = (targetObject);
			predictionCircleGuids_[i] = targetObject->GetGuid();
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
			if(target.lock()) {
				CalyxEngine::Vector3 targetPos = target.lock()->GetWorldPosition();
				targetPos.y = 0.01f;
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

void DroolRainEvent::DeleteDroolObject() {

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			ctx->RemoveObject(predictionCircles_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
			predictionCircles_.resize(objectCount_);
			targetObjectGuids_.resize(objectCount_);
			predictionCircleGuids_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}

void DroolRainEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	targetObjectGuids_ = derived->value("targetObjectGuids", std::vector<Guid>{});
	predictionCircleGuids_ = derived->value("predictionCircleGuids", std::vector<Guid>{});
}

void DroolRainEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	std::vector<Guid> targetGuids;
	targetGuids.reserve(targetObjects_.size());
	for(const auto& target : targetObjects_) {
		if(auto object = target.lock()) targetGuids.push_back(object->GetGuid());
	}
	if(targetGuids.empty()) targetGuids = targetObjectGuids_;
	if(!targetGuids.empty()) derived["targetObjectGuids"] = targetGuids;

	std::vector<Guid> predictionGuids;
	predictionGuids.reserve(predictionCircles_.size());
	for(const auto& prediction : predictionCircles_) {
		if(auto object = prediction.lock()) predictionGuids.push_back(object->GetGuid());
	}
	if(predictionGuids.empty()) predictionGuids = predictionCircleGuids_;
	if(!predictionGuids.empty()) derived["predictionCircleGuids"] = predictionGuids;
}

void DroolRainEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuids(targetObjectGuids_, guidMap);
	RemapGuids(predictionCircleGuids_, guidMap);
}
