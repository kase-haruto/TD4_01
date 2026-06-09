#include "PitfallEvent.h"

#include <Game/DemoPlayer/DemoPlayer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>

REGISTER_SCENE_OBJECT(PitfallEvent)

PitfallEvent::PitfallEvent(const std::string& name) : StageGimmickEventBase(name) {}

void PitfallEvent::SetTarget(const std::shared_ptr<PitfallObject>& target) {
	targetObject_	  = target;
	targetObjectGuid_ = target ? target->GetGuid() : Guid{};
}

void PitfallEvent::OnCollisionEnter(Collider* other) {
	if(!other) {
		return;
	}

	auto* player = dynamic_cast<DemoPlayer*>(other->GetOwner());
	if(!player) {
		return;
	}

	player->StartPitfallRecovery();
	if(param_.disableAfterHit) {
		if(collider_) {
			collider_->SetCollisionEnabled(false);
		}
	}
}

void PitfallEvent::EventInitialize() {

	collider_->SetCollisionEnabled(true);
	if(!hasSerializedParam_) {
		param_.LoadParams();
	}

	if(!targetObject_.expired()) {
		return;
	}

	const std::string eventPrefix  = "PitfallEvent";
	const std::string objectPrefix = "PitfallObject";

	auto object = ResolveLinkedObject<PitfallObject>(targetObjectGuid_, objectPrefix);
	if(!object) object = FindOwnedObjectByClassName<PitfallObject>(objectPrefix);
	if(object) {
		targetObject_ = object;
		object->SetName(objectPrefix);
		SetTarget(object);
		return;
	}
	// シーンから対応するオブジェクトが無ければ生成する
	targetObject_ = SceneAPI::Instantiate<PitfallObject>("plane.obj", objectPrefix);
	targetObject_.lock()->SetParent(shared_from_this());
	targetObjectGuid_ = targetObject_.lock()->GetGuid();
	targetObject_.lock()->Initialize();
	targetObject_.lock()->SetTexture("hole/hole.png");
	targetObject_.lock()->GetWorldTransform().translation.y -= 0.5f;
	targetObject_.lock()->GetWorldTransform().inheritScale = false;
}

void PitfallEvent::EventUpdate(float) {

	if(!targetObject_.lock()) {
		EventInitialize();
	}
}

void PitfallEvent::DerivativeGui() {
	param_.ShowGui();
}

void PitfallEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("param")) {
		param_.ApplyParamsFromJson(derived->at("param"));
		hasSerializedParam_ = true;
	}
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
}

void PitfallEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	param_.ExtractParamsToJson(derived["param"]);
	if(auto target = targetObject_.lock()) {
		derived["targetObjectGuid"] = target->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}
}

void PitfallEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(targetObjectGuid_, guidMap);
}