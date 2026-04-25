#include "BreakableFloorObject.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BreakableFloorObject)

BreakableFloorObject::BreakableFloorObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BreakableFloorObject::Break() {

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

void BreakableFloorObject::ObjectInitialize() {
	isBroken_ = false;
}

void BreakableFloorObject::ObjectUpdate(float dt) {
	dt;
}
