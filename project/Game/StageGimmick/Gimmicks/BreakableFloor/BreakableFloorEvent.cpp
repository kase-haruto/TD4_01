#include "BreakableFloorEvent.h"

#include <Game/DemoPlayer/DemoPlayer.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BreakableFloorEvent)

BreakableFloorEvent::BreakableFloorEvent(const std::string& name) : StageGimmickEventBase(name) {}

void BreakableFloorEvent::SetTarget(const std::shared_ptr<BreakableFloorObject>& target) {
	targetObject_ = target;
	targetObjectGuid_ = target ? target->GetGuid() : Guid{};
}

void BreakableFloorEvent::OnCollisionEnter(Collider* other) {

	// ハンマー判定かどうか確認する
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(!otherObj) {
		return;
	}

	// プレイヤーの攻撃に当たったら床を壊す
	auto floor = targetObject_.lock();
	if(other->GetType() == ColliderType::Type_PlayerAttack) {
		if(!floor) {
			return;
		} else if(!floor->IsBroken()) {
			floor->Break();
		}
	}

	// イベントを無効化する
	isActive_ = false;
}

void BreakableFloorEvent::OnCollisionStay(Collider* other) {

	// プレイヤーが当たったらダメージを出す
	auto floor = targetObject_.lock();
	if(other->GetType() == ColliderType::Type_Player) {
		auto* player = dynamic_cast<DemoPlayer*>(other->GetOwner());
		if(!player || !floor->IsBroken()) {
			return;
		}

		player->StartPitfallRecovery();
		player->TakeDamage(1);
		if(param_.disableAfterHit) {
			if(collider_) {
				collider_->SetCollisionEnabled(false);
			}
		}
	}
}

void BreakableFloorEvent::EventInitialize() {

	if(!hasSerializedParam_) {
		param_.LoadParams();
	}

	if(!targetObject_.expired()) {
		return;
	}

	const std::string eventPrefix  = "BreakableFloorEvent";
	const std::string objectPrefix = "BreakableFloorObject";

	auto object = ResolveLinkedObject<BreakableFloorObject>(targetObjectGuid_, objectPrefix);
	if(!object) object = FindOwnedObjectByClassName<BreakableFloorObject>(objectPrefix);
	if(object) {
		targetObject_ = object;
		object->SetName(objectPrefix);
		SetTarget(object);
	} else {
		// シーンから対応するオブジェクトが無ければ生成する
		targetObject_ = SceneAPI::Instantiate<BreakableFloorObject>("breakableFloor.obj", objectPrefix);
		targetObject_.lock()->SetParent(shared_from_this());
		targetObjectGuid_ = targetObject_.lock()->GetGuid();
		targetObject_.lock()->Initialize();
		targetObject_.lock()->GetWorldTransform().translation.y -= 0.5f;
		targetObject_.lock()->GetWorldTransform().inheritScale = false;
	}

	const std::string pitfallName = "pitfall";
	auto			  pitfall	  = ResolveLinkedObjectByName<GeneralObject>(pitfallGuid_, pitfallName);
	if(!pitfall) pitfall = FindOwnedObjectByName<GeneralObject>(pitfallName);
	if(pitfall) {
		pitfall->SetName(pitfallName);
		pitfall->Initialize();
		pitfall->SetTexture("hole/hole.png");
		pitfall_	 = pitfall;
		pitfallGuid_ = pitfall->GetGuid();
	} else {
		auto newPitfall = SceneAPI::Instantiate<GeneralObject>("plane.obj", pitfallName);
		newPitfall->SetParent(shared_from_this(), false);
		newPitfall->Initialize();
		newPitfall->SetTexture("hole/hole.png");
		newPitfall->SetScale({1.0f, 1.0f, 1.0f});
		newPitfall->SetTranslate({0.0f, -0.5f, 0.0f});
		pitfall_	 = newPitfall;
		pitfallGuid_ = newPitfall->GetGuid();
	}
}

void BreakableFloorEvent::EventUpdate(float) {

	if(!targetObject_.lock()) {
		EventInitialize();
	}
}

void BreakableFloorEvent::DerivativeGui() {
	param_.ShowGui();
}

void BreakableFloorEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("param")) {
		param_.ApplyParamsFromJson(derived->at("param"));
		hasSerializedParam_ = true;
	}
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
}

void BreakableFloorEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	param_.ExtractParamsToJson(derived["param"]);
	if(auto target = targetObject_.lock()) {
		derived["targetObjectGuid"] = target->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}
}

void BreakableFloorEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(targetObjectGuid_, guidMap);
}
