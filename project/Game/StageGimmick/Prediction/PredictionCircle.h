#pragma once

#include "Engine\Objects\3D\Actor\BaseGameObject.h"

// 前方宣言
class DroolRainObject;

/// <summary>
/// 予測円のオブジェクトクラス
/// </summary>
class PredictionCircle : public BaseGameObject {

public:

	PredictionCircle() = default;
	PredictionCircle(
		const std::string& modelName, 
		std::optional<std::string> objectName = std::nullopt);
	~PredictionCircle() override = default;

	void Initialize() override;
	void Update(float dt) override;

	std::string_view GetObjectClassName() const override { return "PredictionCircle"; }

	void SetTargetObject(DroolRainObject* target);

private:

	// よだれのオブジェクト
	DroolRainObject* target_;
	float targetObjectY_ = 0.0f;

};
