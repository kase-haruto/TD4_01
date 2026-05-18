#include "BreakableWallObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BreakableWallObject)

BreakableWallObject::BreakableWallObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BreakableWallObject::Break() {

	// 壊れているなら何もしない
	if(isBroken_) {
		return;
	}
	isBroken_ = true;

	// コライダーとモデルを無効化
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}

	// 破片パーティクル、SE、カメラ揺れを入れる
}

void BreakableWallObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isBroken_ = false;
}

void BreakableWallObject::ObjectUpdate(float) {
}
