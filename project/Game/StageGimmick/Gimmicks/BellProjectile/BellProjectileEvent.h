#pragma once
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Base\GeneralObject.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileObject.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileTarget.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileDoor.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"



CALYX_OBJECT(Category = Event, DisplayName = "BellProjectileEvent")
class BellProjectileEvent : public StageGimmickEventBase {
public:
	BellProjectileEvent() = default;
	BellProjectileEvent(const std::string& name);
	~BellProjectileEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileEvent";
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
	/// 鐘イベント全体のパラメータ
	/// </summary>
	struct BellProjectileEventParam : public CalyxEngine::SerializableObject {

		BellProjectileParam param_;
		BellProjectileTargetParam targetParam_;
		BellProjectileDoorParam doorParam_;

		BellProjectileEventParam() {
			AddField("scale", param_.scale).Category("BellProjectileEvent").Tooltip("撞木の通常スケール");
			AddField("hitScale", param_.hitScale).Category("BellProjectileEvent").Tooltip("判定（パリィ）を取った時のスケール");
			AddField("direction", param_.direction).Category("BellProjectileEvent").Tooltip("調整用：触らないで");
			AddField("speed", param_.speed).Category("BellProjectileEvent").Tooltip("撞木の移動速度");
			AddField("parryDuration", param_.parryDuration).Category("BellProjectileEvent").Tooltip("撞木が鐘に向かっていく時間");

			AddField("targetScale", targetParam_.scale).Category("BellProjectileTarget").Tooltip("鐘の通常スケール");
			AddField("targetHitScale", targetParam_.hitScale).Category("BellProjectileTarget").Tooltip("鐘が鳴った時のスケール");
			AddField("targetMoveSpeed", targetParam_.moveSpeed).Category("BellProjectileTarget").Tooltip("鳴った後に鐘が上へ上がる速度");

			AddField("doorScale", doorParam_.scale).Category("BellProjectileDoor").Tooltip("調整用：触らないで");
			AddField("doorSpeed", doorParam_.speed).Category("BellProjectileDoor").Tooltip("扉が開く速さ");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "BellProjectileEvent", "StageGimmick"};
		}
	};


private:
	// 鐘
	std::weak_ptr<BellProjectileTarget> bell_;
	Guid							bellGuid_;

	// 撞木
	std::weak_ptr<BellProjectileObject> targetObject_;
	Guid								targetObjectGuid_;

	// 扉
	std::weak_ptr<BellProjectileDoor> doorL_;
	Guid							doorLGuid_;
	std::weak_ptr<BellProjectileDoor> doorR_;
	Guid							doorRGuid_;

	// 建物
	std::weak_ptr<GeneralObject> gate_;
	Guid						 gateGuid_;
	std::weak_ptr<GeneralObject>	  wallL_;
	Guid							  wallLGuid_;
	std::weak_ptr<GeneralObject>	  wallR_;
	Guid							  wallRGuid_;

	// パラメータ
	BellProjectileEventParam   eventParam_;
	bool					   hasSerializedEventParam_ = false;
};