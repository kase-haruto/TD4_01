

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\Pitfall\PitfallObject.h"

/// <summary>
/// 落とし穴のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "PitfallEvent")
class PitfallEvent : public StageGimmickEventBase {
public:
	PitfallEvent() = default;
	PitfallEvent(const std::string& name);
	~PitfallEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "PitfallEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<PitfallObject>& target);

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
	/// 落とし穴イベントのパラメータ
	/// </summary>
	struct PitfallEventParam : public CalyxEngine::SerializableObject {
		float yOffsetOnCreate = -0.5f;
		bool  disableAfterHit = true;

		PitfallEventParam() {
			AddField("Y Offset On Create", yOffsetOnCreate).Category("PitfallEvent").Range(-5.0f, 5.0f).Tooltip("関係無し");
			AddField("Disable After Hit", disableAfterHit).Category("PitfallEvent").Tooltip("関係無し");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "PitfallEvent", "StageGimmick"};
		}
	};

private:
	// ターゲットの落とし穴オブジェクト
	std::weak_ptr<PitfallObject> targetObject_;
	Guid						 targetObjectGuid_;

	// 調整項目
	PitfallEventParam param_;
	bool			  hasSerializedParam_ = false;
};