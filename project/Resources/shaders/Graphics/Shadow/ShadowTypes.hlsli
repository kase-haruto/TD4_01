#pragma once

struct ShadowParams {
	float4x4 gLightVP;
	float gShadowBias;
	float3 _shadowPad;
};

struct RaytracingShadowParams {
	float gShadowRayEps;
	float gBaseAngularRadius;
	float gMinShadow;
	int gIsSoft;
};