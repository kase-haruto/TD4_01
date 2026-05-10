#pragma once

#include "Engine\Foundation\Math\Vector3.h"

/// <summary>
/// よだれ雨のパラメータ
/// </summary>
struct DroolRainParam {

	// 落下中の速度
	float velocityY_	 = -0.2f;
	float accelerationY_ = -1.5f;

	// 空中でのスケール変化速度
	float airScaleSpeed_ = 0.0f;

	// 地面に付いた後の時間
	float groundLifeTime_ = 0.0f;

	// 地面に付いた後のスケール変化速度
	CalyxEngine::Vector3 groundScaleSpeed_ = {};

	// 地面に付いた後の速度
	float groundVelocityY_ = 0.0f;
};