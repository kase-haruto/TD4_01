#pragma once
#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 鐘オブジェクト
/// </summary>
class BellProjectileTarget : public StageGimmickObjectBase {
public:
	BellProjectileTarget() = default;
	BellProjectileTarget(const std::string&			modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~BellProjectileTarget() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileTarget";
	}

	void SetParam(const BellProjectileTargetParam& param) {
		param_ = param;
	}

	void SetIsRing(bool ring) { isRing_ = ring; }
	bool GetIsRing() const { return isRing_; }
	// 鐘部分の座標を取得
	const CalyxEngine::Vector3 GetTargetPos() const;

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:
	void ChangeScale() {
		CalyxEngine::Vector3 targetScale = isRing_ ? param_.hitScale : param_.scale;
		worldTransform_.scale = CalyxEngine::Vector3::Lerp(
			worldTransform_.scale, targetScale, 0.1f);
	}

private:
	// パラメータ
	BellProjectileTargetParam param_;

	// 鐘がなったか（扉が開くか）
	bool isRing_ = false;

};
