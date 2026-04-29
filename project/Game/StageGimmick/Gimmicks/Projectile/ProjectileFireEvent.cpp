#include "ProjectileFireEvent.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(ProjectileFireEvent)

ProjectileFireEvent::ProjectileFireEvent(const std::string& name) : StageGimmickEventBase(name) {}

void ProjectileFireEvent::SetTarget(const std::shared_ptr<ProjectileObject>& target) {
	targetObject_ = target;
}

void ProjectileFireEvent::OnCollisionEnter(Collider* other) {

	other;

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら飛んでくる
	if(auto target = targetObject_.lock()) {
		target->SetIsFlying(true);
		target->SetIsHoming(isHoming_);
	}
}

void ProjectileFireEvent::EventInitialize() {

	if(!targetObject_.expired()) {
		return;
	}

	std::string eventName = GetName();

	const std::string eventPrefix  = "ProjectileEvent";
	const std::string objectPrefix = "ProjectileObject";
	// イベント名が"ProjectileEvent"で始まっているか確認する
	if(eventName.find(eventPrefix) != 0) {
		return;
	}
	// 番号を抜き取る
	std::string suffix = eventName.substr(eventPrefix.size());
	// 対応するオブジェクト名を作る
	std::string targetName = objectPrefix + suffix;
	// シーンから対応するオブジェクトを探す
	targetObject_ =
		SceneContext::Current()->FindObjectByName<ProjectileObject>(targetName);
}

void ProjectileFireEvent::EventUpdate(float dt) {

	dt;
}
