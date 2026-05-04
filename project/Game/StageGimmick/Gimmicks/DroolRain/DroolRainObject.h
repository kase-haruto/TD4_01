#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"

/// <summary>
/// よだれ雨オブジェクト
/// </summary>
class DroolRainObject : public StageGimmickObjectBase 
{
public:

	DroolRainObject() = default;
	DroolRainObject(const std::string& modelName,
					std::optional<std::string> objectName = std::nullopt);
	~DroolRainObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "DroolRainObject";
	}

	void SetIsRaining(bool isRaining) {
		isRaining_ = isRaining;
	}

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 雨が降っているか
	bool isRaining_ = false;
};
