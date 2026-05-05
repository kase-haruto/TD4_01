#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <cstdint>
#include <string>
struct MaterialConfig final{
	 //========================= variable =========================
	CalyxEngine::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };	//< 色
	int32_t enableLighting = 0;					//< ライティング有無
	float shininess = 20.0f;					//< 光沢
	float enviromentCoefficient = 0.5f;			//< 環境光の強さ
	bool isReflect = false;						//< 反射
	float roughness = 0.5f;						//< 反射の粗さ
	int currentLightingMode_ = 0;				//< ライティングモード
	CalyxEngine::Vector4 toonHighlightColor = { 1.15f, 1.10f, 1.00f, 1.0f };
	CalyxEngine::Vector4 toonBaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	CalyxEngine::Vector4 toonMidShadowColor = { 0.72f, 0.76f, 0.86f, 1.0f };
	CalyxEngine::Vector4 toonShadowColor = { 0.42f, 0.46f, 0.58f, 1.0f };
	float toonThreshold1 = -0.15f;
	float toonThreshold2 = 0.25f;
	float toonThreshold3 = 0.82f;
	float toonEdgeSoftness = 0.03f;
	float toonSpecularThreshold = 0.96f;
	float toonSpecularSoftness = 0.02f;
	float toonSpecularIntensity = 0.35f;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MaterialConfig,
								   color,
								   enableLighting,
								   shininess,
								   enviromentCoefficient,
								   isReflect,
								   roughness,
								   currentLightingMode_,
								   toonHighlightColor,
								   toonBaseColor,
								   toonMidShadowColor,
								   toonShadowColor,
								   toonThreshold1,
								   toonThreshold2,
								   toonThreshold3,
								   toonEdgeSoftness,
								   toonSpecularThreshold,
								   toonSpecularSoftness,
								   toonSpecularIntensity)
