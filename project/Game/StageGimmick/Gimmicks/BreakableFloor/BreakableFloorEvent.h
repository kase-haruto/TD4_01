#pragma once

#include "Game\StageGimmick\Gimmicks\BreakableFloor\BreakableFloorObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"

/// <summary>
/// 壊れる床のイベントクラス
/// </summary>
class BreakableFloorEvent : public StageGimmickEventBase
{
public:

	BreakableFloorEvent() = default;
	BreakableFloorEvent(const std::string& name);
	~BreakableFloorEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "BreakableFloorEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<BreakableFloorObject>& target);

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

private:

	// ターゲットの壊れる床オブジェクト
	std::weak_ptr<BreakableFloorObject> targetObject_;

};
