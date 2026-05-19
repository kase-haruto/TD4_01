#pragma once

#include "Engine\Foundation\Math\Vector3.h"

/// <summary>
/// よだれ雨のパラメータ
/// </summary>
struct DroolRainParam {

	// オブジェクトのスケール
	float objectScale_ = 1.0f;
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

/// <summary>
/// 地面から生えてくる歯のパラメータ
/// </summary>
struct GroundSpikeParam {

	// オブジェクトのスケール
	float objectScale = 1.0f;
	/// 飛び出す速度
	float popUpSpeed = 15.0f;
	/// 飛び出す高さ
	float popUpHeight = 3.0f;

};

/// <summary>
/// サイコロのイベントのパラメータ
/// </summary>
struct DiceProjectileParam {

	// オブジェクトのスケール
	float scale = 1.0f;
	// 判定を取った際のスケール
	float hitScale = 1.0f;
	// 飛んでいく方向
	CalyxEngine::Vector3 direction = -CalyxEngine::Vector3::Forward();
	// サイコロの速度
	float speed;
	// サイコロの回転スピード
	float rotateSpeed;
	// サイコロが収納箱に向かっていく時間
	float parryDuration;

};

/// <summary>
/// 炎のパラメーター
/// </summary>
struct FireLoadParam {
	// 出現時の大きな燃え上がり時間
	float time = 0.75f;
	// 出現時の燃え上がる高さ
	float burnHeight = 5.0f;
	// 幅の数 (横方向)
	int width = 8;
	// 列の数 (奥行き)
	int depth = 2;
};

struct BellProjectileParam {

	// オブジェクトのスケール
	CalyxEngine::Vector3 scale = {1.0f, 1.0f, 2.0f};
	// 判定を取った際のスケール
	CalyxEngine::Vector3 hitScale = {1.5f, 1.5f, 2.0f};
	// 飛んでいく方向
	CalyxEngine::Vector3 direction = -CalyxEngine::Vector3::Forward();
	// 撞木の速度
	float speed = 1.5f;
	// 撞木が鐘に向かっていく時間
	float parryDuration = 0.7f;
};

struct BellProjectileTargetParam {

	// オブジェクトのスケール
	CalyxEngine::Vector3 scale = {2.0f, 4.0f, 2.0f};
	// 鳴った時のスケール
	CalyxEngine::Vector3 hitScale = {2.5f, 4.0f, 2.5f};
	// 鳴った後に上に上がるスピード
	float moveSpeed = 5.0f;
};

struct BellProjectileDoorParam {

	// オブジェクトのスケール
	CalyxEngine::Vector3 scale = {17.0f, 5.0f, 1.0f};
	// 開く時の速さ
	float speed = 2.0f;
};