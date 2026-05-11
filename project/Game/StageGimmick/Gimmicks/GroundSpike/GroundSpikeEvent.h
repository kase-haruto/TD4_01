#pragma once

#include "Game\StageGimmick\Gimmicks\GroundSpike\GroundSpikeObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 地面から生えてくる歯のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "GroundSpikeEvent")
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

	// gui
	void DerivativeGui() override;

private:
	/// <summary>
	/// 地面から生えてくる歯イベント全体のパラメータ
	/// </summary>
	struct GroundSpikeEventParam : public CalyxEngine::SerializableObject {

		GroundSpikeParam param_;

		GroundSpikeEventParam() {
			AddField("popUpSpeed", param_.popUpSpeed).Category("GroundSpikeEvent");
			AddField("popUpHeight", param_.popUpHeight).Category("GroundSpikeEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "GroundSpikeEvent", "StageGimmick"};
		}
	};

private:

	// ターゲットの地面スパイクオブジェクト
	std::weak_ptr<GroundSpikeObject> targetObject_;


	GroundSpikeEventParam eventParam_;

};
