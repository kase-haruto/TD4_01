#include "StageGimmickObjectBase.h"

StageGimmickObjectBase::StageGimmickObjectBase
(const std::string& modelName, std::optional<std::string> objectName) : 
	BaseGameObject(modelName, objectName) {}

void StageGimmickObjectBase::Initialize() {
	// 実体オブジェの初期化
	BaseGameObject::Initialize();
	ObjectInitialize();
}

void StageGimmickObjectBase::Update(float dt) {

	// 実体オブジェの更新
	BaseGameObject::Update(dt);

	// アクティブが無ければ更新止める
	if(!isActive_) {
		return;
	}

	// 実体オブジェの更新
	ObjectUpdate(dt);
}
