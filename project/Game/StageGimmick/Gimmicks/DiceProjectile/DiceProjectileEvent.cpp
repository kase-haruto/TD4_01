#include "DiceProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

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

	const std::string eventPrefix  = "DiceProjectileEvent";
	const std::string objectName = "DiceProjectileObject";
	const std::string socketPrefix = "DiceSocketObject";

	if(GetName() != eventPrefix) {
		return;
	}

	auto object = ResolveLinkedObject<DiceSocketObject>(socketGuid_, socketPrefix);
	if(!object) object = FindOwnedObjectByClassName<DiceSocketObject>(socketPrefix);
	if(object) {
		object->SetName(socketPrefix);
		socket_ = object;
		socketGuid_ = object->GetGuid();

	// シーンから対応するオブジェクトが無ければ生成する
	} else {
		socket_ = SceneAPI::Instantiate<DiceSocketObject>("diceStorage.obj", socketPrefix);
		socketGuid_ = socket_.lock()->GetGuid();
	}
	socket_.lock()->SetParent(shared_from_this());
	socket_.lock()->SetClearCount(eventData_.clearCount);
	socket_.lock()->SetCrackerPos(eventParam_.param_.crackerPos);
	socket_.lock()->SetCrackerInterval(eventParam_.param_.crackerInterval);
	socket_.lock()->Initialize();

	CreateDoors();
	UpdateDoorOpenRequest();

	auto childTargets = ResolveLinkedObjects<DiceProjectileObject>(targetObjectGuids_, objectName);
	if(childTargets.empty()) childTargets = FindOwnedObjectsByClassName<DiceProjectileObject>(objectName);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
		eventData_.objectCount = objectCount_;
		targetObjects_.resize(objectCount_);
		targetObjectGuids_.resize(objectCount_);

		for(size_t i = 0; i < childTargets.size(); ++i) {
			targetObjects_[i] = childTargets[i];
			targetObjectGuids_[i] = childTargets[i]->GetGuid();
			childTargets[i]->SetName(objectName);
			childTargets[i]->SetParam(eventParam_.param_);
			childTargets[i]->SetSocket(socket_.lock().get());
			childTargets[i]->Initialize();
		}
		return;
	}

	targetObjects_.resize(objectCount_);
	targetObjectGuids_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto targetObject = SceneAPI::Instantiate<DiceProjectileObject>("dice.obj", objectName);
		if(targetObject) {
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->SetSocket(socket_.lock().get());
			targetObject->Initialize();
			targetObjects_[i] = (targetObject);
			targetObjectGuids_[i] = targetObject->GetGuid();
		}
	}
}

void DiceProjectileEvent::EventUpdate(float) {

	if(targetObjects_.empty()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		if(!hasSerializedEventData_) {
			eventData_.SetEventName(GetName());
			eventData_.LoadParams();
		}
		objectCount_ = eventData_.objectCount;
		EventInitialize();
	}

	UpdateDoorOpenRequest();
}

void DiceProjectileEvent::DerivativeGui() {

	ImGui::Text("DiceProjectileObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		AddDiceProjectileObject();
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

void DiceProjectileEvent::AddDiceProjectileObject() {

	const std::string objectName   = "DiceProjectileObject";
	auto targetObject = SceneAPI::Instantiate<DiceProjectileObject>("dice.obj", objectName);
	if(!targetObject) return;

	targetObject->SetParent(shared_from_this());
	targetObject->SetParam(eventParam_.param_);
	targetObject->SetSocket(socket_.lock().get());
	targetObject->Initialize();

	targetObjects_.push_back(targetObject);
	++objectCount_;
	eventData_.objectCount = objectCount_;
}

void DiceProjectileEvent::DeleteDroolObject() {

	--objectCount_;
	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
			targetObjectGuids_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}

void DiceProjectileEvent::CreateDoors() {

	// 扉(左)
	const std::string doorLName = "DiceProjectileDoorL";
	auto			  doorL		= ResolveLinkedObject<BellProjectileDoor>(doorLGuid_, doorLName);
	if(!doorL) {
		for(const auto& child : GetChildren()) {
			if(child && child->GetName() == doorLName) {
				doorL = std::dynamic_pointer_cast<BellProjectileDoor>(child);
				break;
			}
		}
	}
	if(doorL) {
		doorL->SetName(doorLName);
		doorL->SetLR(0);
		doorL->SetParam(eventParam_.doorParam_);
		doorL->Initialize();
		doorL_	   = doorL;
		doorLGuid_ = doorL->GetGuid();
	} else {
		auto newDoor = SceneAPI::Instantiate<BellProjectileDoor>("sanmonDoor.obj", doorLName);
		newDoor->SetParent(shared_from_this(), false);
		newDoor->SetLR(0);
		newDoor->SetParam(eventParam_.doorParam_);
		newDoor->Initialize();
		newDoor->SetTranslate({-12.0f, 0.0f, 40.0f});
		doorL_	   = newDoor;
		doorLGuid_ = newDoor->GetGuid();
	}

	// 扉(右)
	const std::string doorRName = "DiceProjectileDoorR";
	auto			  doorR		= ResolveLinkedObject<BellProjectileDoor>(doorRGuid_, doorRName);
	if(!doorR) {
		for(const auto& child : GetChildren()) {
			if(child && child->GetName() == doorRName) {
				doorR = std::dynamic_pointer_cast<BellProjectileDoor>(child);
				break;
			}
		}
	}
	if(doorR) {
		doorR->SetName(doorRName);
		doorR->SetLR(1);
		doorR->SetParam(eventParam_.doorParam_);
		doorR->Initialize();
		doorR_	   = doorR;
		doorRGuid_ = doorR->GetGuid();
	} else {
		auto newDoor = SceneAPI::Instantiate<BellProjectileDoor>("sanmonDoorR.obj", doorRName);
		newDoor->SetParent(shared_from_this(), false);
		newDoor->SetLR(1);
		newDoor->SetParam(eventParam_.doorParam_);
		newDoor->Initialize();
		newDoor->SetTranslate({12.0f, 0.0f, 40.0f});
		doorR_	   = newDoor;
		doorRGuid_ = newDoor->GetGuid();
	}
}

void DiceProjectileEvent::UpdateDoorOpenRequest() {
	bool shouldOpen = false;
	if(auto socket = socket_.lock()) {
		shouldOpen = socket->GetDiceSocketCount() >= static_cast<uint32_t>(eventData_.clearCount);
	}

	if(auto doorL = doorL_.lock()) {
		doorL->SetOpenRequested(shouldOpen);
	}
	if(auto doorR = doorR_.lock()) {
		doorR->SetOpenRequested(shouldOpen);
	}
}

void DiceProjectileEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	if(derived->contains("eventData")) {
		eventData_.ApplyParamsFromJson(derived->at("eventData"));
		objectCount_ = eventData_.objectCount;
		hasSerializedEventData_ = true;
	}
	socketGuid_ = derived->value("socketGuid", Guid{});
	targetObjectGuids_ = derived->value("targetObjectGuids", std::vector<Guid>{});
	doorLGuid_		   = derived->value("doorLGuid", Guid{});
	doorRGuid_		   = derived->value("doorRGuid", Guid{});
	for(auto& target : targetObjects_) {
		if(auto object = target.lock()) {
			object->SetParam(eventParam_.param_);
		}
	}
	if(auto doorL = doorL_.lock()) {
		doorL->SetParam(eventParam_.doorParam_);
	}
	if(auto doorR = doorR_.lock()) {
		doorR->SetParam(eventParam_.doorParam_);
	}
}

void DiceProjectileEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	eventData_.ExtractParamsToJson(derived["eventData"]);
	if(auto socket = socket_.lock()) {
		derived["socketGuid"] = socket->GetGuid();
	} else if(socketGuid_.isValid()) {
		derived["socketGuid"] = socketGuid_;
	}

	std::vector<Guid> guids;
	guids.reserve(targetObjects_.size());
	for(const auto& target : targetObjects_) {
		if(auto object = target.lock()) guids.push_back(object->GetGuid());
	}
	if(guids.empty()) guids = targetObjectGuids_;
	if(!guids.empty()) derived["targetObjectGuids"] = guids;

	if(auto doorL = doorL_.lock()) {
		derived["doorLGuid"] = doorL->GetGuid();
	} else if(doorLGuid_.isValid()) {
		derived["doorLGuid"] = doorLGuid_;
	}

	if(auto doorR = doorR_.lock()) {
		derived["doorRGuid"] = doorR->GetGuid();
	} else if(doorRGuid_.isValid()) {
		derived["doorRGuid"] = doorRGuid_;
	}
}

void DiceProjectileEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(socketGuid_, guidMap);
	RemapGuids(targetObjectGuids_, guidMap);
	RemapGuid(doorLGuid_, guidMap);
	RemapGuid(doorRGuid_, guidMap);
}