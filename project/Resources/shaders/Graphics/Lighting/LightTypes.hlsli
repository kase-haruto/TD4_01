#pragma once

struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

struct PointLight {
	float4 color;
	float3 position;
	float intensity;
	float radius;
	float decay;
	float2 _pointLightPad;
};

