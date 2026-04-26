#pragma once

#include "Game\StageGimmick\Gimmicks\GroundSpike\GroundSpikeObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"

/// <summary>
/// 地面から生えてくる歯のイベントクラス
/// </summary>
class GroundSpikeEvent : public StageGimmickEventBase 
{
public:

	GroundSpikeEvent() = default;
	GroundSpikeEvent(const std::string& name);
	~GroundSpikeEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "GroundSpikeEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<GroundSpikeObject>& target);

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

private:

	// ターゲットの地面スパイクオブジェクト
	std::weak_ptr<GroundSpikeObject> targetObject_;

};
