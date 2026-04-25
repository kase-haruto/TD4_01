#include "StageGimmickEventBase.h"

StageGimmickEventBase::StageGimmickEventBase(const std::string& name) : BaseEventObject(name) {}

void StageGimmickEventBase::Initialize() {

	// イベントオブジェの初期化
	BaseEventObject::Initialize();
	EventInitialize();
}

void StageGimmickEventBase::AlwaysUpdate(float dt) {

	// イベントオブジェの更新
	BaseEventObject::AlwaysUpdate(dt);

	// アクティブが無ければ更新止める
	if(!isActive_) { return; }

	// イベントの更新
	EventUpdate(dt);
}