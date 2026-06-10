#pragma once

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\DiceProjectile\DiceSocketObject.h"
#include "Game\StageGimmick\Gimmicks\DiceProjectile\DiceProjectileObject.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileDoor.h"
#include "Game\StageGimmick\Base\GeneralObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// サイコロが飛んでくるイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "DiceProjectileEvent")
class DiceProjectileEvent : public StageGimmickEventBase
{
public:

	DiceProjectileEvent() = default;
	DiceProjectileEvent(const std::string& name);
	~DiceProjectileEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "DiceProjectileEvent";
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
	/// サイコロのイベント全体のパラメータ
	/// </summary>
	struct DiceProjectileEventParam : public CalyxEngine::SerializableObject {

		DiceProjectileParam param_;
		BellProjectileDoorParam doorParam_;

		DiceProjectileEventParam() {
			AddField("scale", param_.scale).Category("DiceProjectileEvent").Tooltip("サイコロのデフォルトスケール");
			AddField("hitScale", param_.hitScale).Category("DiceProjectileEvent").Tooltip("サイコロがヒットしたときのスケール");
			AddField("rotateSpeed", param_.rotateSpeed).Category("DiceProjectileEvent").Tooltip("サイコロの回転速度");
			AddField("speed", param_.speed).Category("DiceProjectileEvent").Tooltip("サイコロの速度");
			AddField("parryDuration", param_.parryDuration).Category("DiceProjectileEvent").Tooltip("サイコロがソケットに入るまでの時間");
			AddField("direction", param_.direction).Category("DiceProjectileEvent").Tooltip("サイコロの飛ぶ方向");

			AddField("crackerPos", param_.crackerPos).Category("DiceProjectileEvent").Tooltip("クラッカーの位置");
			AddField("crackerInterval", param_.crackerInterval).Category("DiceProjectileEvent").Tooltip("クラッカーの間隔");
			
			AddField("doorScale", doorParam_.scale).Category("DiceProjectileDoor").Tooltip("扉のデフォルトスケール");
			AddField("doorSpeed", doorParam_.speed).Category("DiceProjectileDoor").Tooltip("扉の速度");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DiceProjectileEvent", "StageGimmick"};
		}
	};

	/// <summary>
	/// サイコロのイベント個々のパラメータ
	/// </summary>
	struct AllDiceProjectileEventData : public CalyxEngine::SerializableObject {

		int objectCount = 1;
		int clearCount	= 1;
		std::string name_;

		AllDiceProjectileEventData() {
			AddField("ObjectCount", objectCount).ReadOnly().Tooltip("このイベントのサイコロの数");
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
	void AddDiceProjectileObject();
	// 削除用関数
	void DeleteDroolObject();
	// 扉生成用関数
	void CreateDoors();
	// 扉開閉更新用関数
	void UpdateDoorOpenRequest(float dt);

private:
	
	// サイコロの収納箱
	std::weak_ptr<DiceSocketObject> socket_;
	Guid socketGuid_;
	int								openCount_ = 0;

	// ターゲットのサイコロオブジェクト
	std::vector<std::weak_ptr<DiceProjectileObject>> targetObjects_;
	std::vector<Guid> targetObjectGuids_;

	// 扉
	std::weak_ptr<BellProjectileDoor> doorL_;
	Guid							  doorLGuid_;
	std::weak_ptr<BellProjectileDoor> doorR_;
	Guid							  doorRGuid_;

	// 建物
	std::weak_ptr<GeneralObject> gate_;
	Guid						 gateGuid_;
	std::weak_ptr<GeneralObject> wallL_;
	Guid						 wallLGuid_;
	std::weak_ptr<GeneralObject> wallR_;
	Guid						 wallRGuid_;
	std::weak_ptr<GeneralObject> numbersUi_;
	Guid						 numbersUiGuid_;
	float clearTime_ = 0.0f;

	// パラメータ
	DiceProjectileEventParam eventParam_;
	AllDiceProjectileEventData eventData_;
	bool hasSerializedEventParam_ = false;
	bool hasSerializedEventData_ = false;

};
