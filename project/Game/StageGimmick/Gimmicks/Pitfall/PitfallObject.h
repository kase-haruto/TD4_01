

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"

#include "Engine\Scene\Utility\SceneUtility.h"

/// <summary>
/// 落とし穴オブジェクトのクラス
/// </summary>
class PitfallObject : public StageGimmickObjectBase {
public:
	PitfallObject() = default;
	PitfallObject(const std::string&		 modelName,
				  std::optional<std::string> objectName = std::nullopt);
	~PitfallObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "PitfallObject";
	}

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	CalyxEngine::EffectAsset effectData_;
	bool drawEffect_ = false;

};