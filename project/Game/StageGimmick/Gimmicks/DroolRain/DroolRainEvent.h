#pragma once

#include <vector>

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\DroolRain\DroolRainObject.h"
#include "Game\StageGimmick\Prediction\PredictionCircle.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// よだれ雨のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "DroolRainEvent")
class DroolRainEvent : public StageGimmickEventBase
{
public:

	DroolRainEvent() = default;
	DroolRainEvent(const std::string& name);
	~DroolRainEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "DroolRainEvent";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<DroolRainObject>& target);

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

	// gui
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;
	void RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) override;

private:

	/// <summary>
	/// よだれのイベント全体のパラメータ
	/// </summary>
	struct DroolRainEventParam : public CalyxEngine::SerializableObject {

		DroolRainParam param_;

		DroolRainEventParam() {
			AddField("VelocityY", param_.velocityY_).Category("DroolRainEvent").Tooltip("落下中の初速");
			AddField("AccelerationY", param_.accelerationY_).Category("DroolRainEvent").Tooltip("落下中の加速度");
			AddField("AirScaleSpeed", param_.airScaleSpeed_).Category("DroolRainEvent").Tooltip("調整用：触らないで");
			AddField("GroundLifeTime", param_.groundLifeTime_).Category("DroolRainEvent").Tooltip("地面に付いた後の生存時間");
			AddField("GroundScaleSpeed", param_.groundScaleSpeed_).Category("DroolRainEvent").Tooltip("地面に付いた後のスケール変化速度");
			AddField("GroundVelocityY", param_.groundVelocityY_).Category("DroolRainEvent").Tooltip("地面に付いた後のY速度（広がり/沈み込み）");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DroolRainEvent", "StageGimmick"};
		}
	};
	/// <summary>
	/// よだれのイベント個々のパラメータ
	/// </summary>
	struct AllDroolRainEventData : public CalyxEngine::SerializableObject {

		int objectCount = 1;
		std::string name_;

		AllDroolRainEventData() {
			AddField("ObjectCount", objectCount).ReadOnly();
		}

		void SetEventName(const std::string& name) {
			name_ = name;
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, name_ + "ObjectCount", "StageGimmick"};
		}
	};

	// 追加用関数
	void AddDroolObject();
	// 削除用関数
	void DeleteDroolObject();

private:

	// ターゲットのよだれ雨オブジェクト
	std::vector<std::weak_ptr<DroolRainObject>> targetObjects_;
	std::vector<std::weak_ptr<PredictionCircle>> predictionCircles_;
	std::vector<Guid> targetObjectGuids_;
	std::vector<Guid> predictionCircleGuids_;

	DroolRainEventParam eventParam_;
	AllDroolRainEventData eventData_;
	bool hasSerializedEventParam_ = false;
	bool hasSerializedObjectCount_ = false;
};
