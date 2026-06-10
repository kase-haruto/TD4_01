#pragma once

#include <array>

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\Shoji\ShojiObject.h"
#include "Game\StageGimmick\Gimmicks\Shoji\LuckyCatObject.h"
#include "Game\StageGimmick\Base\GeneralObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 障子のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "ShojiEvent")
class ShojiEvent : public StageGimmickEventBase
{
public:

	ShojiEvent() = default;
	ShojiEvent(const std::string& name);
	~ShojiEvent() = default;

	std::string_view GetObjectClassName() const override {
		return "ShojiEvent";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;
	
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
	/// 障子のイベント全体のパラメータ
	/// </summary>
	struct ShojiEventParam : public CalyxEngine::SerializableObject {

		ShojiParam param_;

		ShojiEventParam() {
			AddField("centerPos", param_.centerPos).Speed(0.01f).Category("ShojiEvent").Tooltip("中心位置");
			AddField("shojiInterval", param_.shojiInterval).Speed(0.01f).Category("ShojiEvent").Tooltip("障子の間隔");
			AddField("wInterval", param_.wInterval).Speed(0.01f).Category("ShojiEvent").Tooltip("障子の紙と紙の幅の間隔");
			AddField("hInterval", param_.hInterval).Speed(0.01f).Category("ShojiEvent").Tooltip("障子の紙と紙の高さの間隔");

			AddField("openTimer", param_.openTimer).Speed(0.01f).Category("ShojiEvent").Tooltip("障子が開く時間");
			AddField("openVelocityX", param_.openVelocityX).Speed(0.01f).Category("ShojiEvent").Tooltip("障子が開く速度X");
			AddField("openAccelerationX", param_.openAccelerationX).Speed(0.01f).Category("ShojiEvent").Tooltip("障子が開く加速度X");
			AddField("wOpen", param_.wOpen).Speed(0.01f).Category("ShojiEvent").Tooltip("障子が開く幅");

			AddField("shojiScale", param_.shojiScale).Category("ShojiEvent").Tooltip("障子のデフォルトスケール");
			AddField("luckyCatScale", param_.luckyCatScale).Category("ShojiEvent").Tooltip("招き猫のデフォルトスケール");
			AddField("hitScale", param_.hitScale).Category("ShojiEvent").Tooltip("招き猫のヒット時のスケール");
			AddField("direction", param_.direction).Category("ShojiEvent").Tooltip("招き猫が進む方向");

			AddField("speed", param_.speed).Category("ShojiEvent").Tooltip("招き猫の速度");
			AddField("parryDuration", param_.parryDuration).Category("ShojiEvent").Tooltip("招き猫が障子に刺さるまでの時間");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "ShojiEvent", "StageGimmick"};
		}
	};

	/// <summary>
	/// 障子のイベント個々のパラメータ
	/// </summary>
	struct AllShojiEventData : public CalyxEngine::SerializableObject {

		int objectCount = 1;
		int clearCount	= 1;
		std::string name_;

		AllShojiEventData() {
			AddField("ObjectCount", objectCount).ReadOnly().Tooltip("このイベントの招き猫の数");
			AddField("ClearCount", clearCount).Tooltip("このイベントのクリアの数");
		}

		void SetEventName(const std::string& name) {
			name_ = name;
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, name_ + "ObjectCount", "StageGimmick"};
		}
	};

	// 追加用関数
	void AddLuckCatObject();
	// 削除用関数
	void DeleteLuckCatObject();

private:

private:

	// 障子のオブジェクト
	std::array<std::shared_ptr<ShojiObject>, 2> shojiObjs_;
	std::array<Guid, 2> shojiGuids_;

	// 招き猫のオブジェクト
	std::vector<std::weak_ptr<LuckyCatObject>> luckyCatObjs_;
	std::vector<Guid> luckyCatGuids_;

	std::weak_ptr<GeneralObject> wallL_;
	Guid						 wallLGuid_;
	std::weak_ptr<GeneralObject> wallR_;
	Guid						 wallRGuid_;
	// はめる数を表示するUI
	std::weak_ptr<GeneralObject> numbersUi_;
	Guid						 numbersUiGuid_;

	// イベントのパラメータ
	ShojiEventParam eventParam_;
	AllShojiEventData eventData_;
	bool hasSerializedEventParam_ = false;
	bool hasSerializedEventData_  = false;

	// 障子が開いているか
	int	 openCount_ = 0;
	bool isOpen_ = false;
	float clearTime_ = 0.0f;
};
