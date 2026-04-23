#pragma once

struct Material {
	float4 color;
	int enableLighting;
	float3 _materialPad0;

	float4x4 uvTransform;

	float shiniess;
	int isReflect;
	float environmentCoefficient;
	float roughness;
};