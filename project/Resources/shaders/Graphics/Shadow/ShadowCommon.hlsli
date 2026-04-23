#pragma once
#include "ShadowTypes.hlsli"

float Hash21(float2 p) {
	float3 p3 = frac(float3(p.xyx) * 0.1031f);
	p3 += dot(p3, p3.yzx + 33.33f);
	return frac((p3.x + p3.y) * p3.z);
}

float2 Rotate2D(float2 v, float a) {
	float s, c;
	sincos(a, s, c);
	return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b) {
	float3 up = (abs(n.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
	t = normalize(cross(up, n));
	b = cross(n, t);
}

static const float2 kPoisson16[16] = {
	float2(-0.326f, -0.406f),
    float2(-0.840f, -0.074f),
    float2(-0.696f, 0.457f),
    float2(-0.203f, 0.621f),
    float2(0.962f, -0.195f),
    float2(0.473f, -0.480f),
    float2(0.519f, 0.767f),
    float2(0.185f, -0.893f),
    float2(-0.433f, 0.043f),
    float2(0.267f, 0.271f),
    float2(-0.123f, -0.754f),
    float2(0.735f, 0.128f),
    float2(0.078f, 0.912f),
    float2(-0.917f, 0.381f),
    float2(0.621f, -0.727f),
    float2(-0.589f, -0.201f)
};

bool TraceVisibilityAnyHit(
    RaytracingAccelerationStructure rtScene,
    float3 origin,
    float3 dir,
    float tMax
) {
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = dir;
	ray.TMin = 0.0f;
	ray.TMax = tMax;

	RayQuery <
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
    > q;

	q.TraceRayInline(
        rtScene,
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF,
        ray
    );

	q.Proceed();
	return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
}

float ComputeDirectionalHardShadowRT(
    RaytracingAccelerationStructure rtScene,
    RaytracingShadowParams shadowParams,
    float3 worldPos,
    float3 normal,
    float3 L
) {
	float3 origin = worldPos + normal * shadowParams.gShadowRayEps;
	const float tMax = 1000.0f;

	bool hit = TraceVisibilityAnyHit(rtScene, origin, L, tMax);
	return hit ? shadowParams.gMinShadow : 1.0f;
}

float ComputeDirectionalSoftShadowRT(
    RaytracingAccelerationStructure rtScene,
    RaytracingShadowParams shadowParams,
    float3 worldPos,
    float3 normal,
    float3 L
) {
	if (shadowParams.gIsSoft == 0) {
		return 1.0f;
	}

	float3 origin = worldPos + normal * shadowParams.gShadowRayEps;
	const float tMax = 1000.0f;

	bool centerHit = TraceVisibilityAnyHit(rtScene, origin, L, tMax);
	if (!centerHit) {
		return 1.0f;
	}

	float angularRadius = max(shadowParams.gBaseAngularRadius, 0.0f);

	int sampleCount = 16;
	if (angularRadius < 0.010f) {
		sampleCount = 4;
	}
	else if (angularRadius < 0.025f) {
		sampleCount = 8;
	}

	float ndotl = saturate(dot(normal, L));
	if (ndotl < 0.35f) {
		sampleCount = min(sampleCount, 4);
	}
	else if (ndotl < 0.60f) {
		sampleCount = min(sampleCount, 8);
	}

	int extraCount = max(sampleCount - 1, 0);

	float3 T, B;
	BuildOrthonormalBasis(L, T, B);

	float rnd = Hash21(worldPos.xz * 17.0f + worldPos.yy * 3.0f);
	float ang = rnd * 6.2831853f;

	float occluded = 1.0f;
	const float kDarkEarlyOut = 0.75f;

    [loop]
	for (int i = 0; i < extraCount; ++i) {
		float2 d = Rotate2D(kPoisson16[i], ang) * angularRadius;
		float3 dirJ = normalize(L + T * d.x + B * d.y);

		bool hit = TraceVisibilityAnyHit(rtScene, origin, dirJ, tMax);
		occluded += hit ? 1.0f : 0.0f;

		if (occluded >= (float) sampleCount * kDarkEarlyOut) {
			return shadowParams.gMinShadow;
		}
	}

	float shadow = 1.0f - (occluded / (float) sampleCount);
	shadow = lerp(shadowParams.gMinShadow, 1.0f, shadow);

	return shadow;
}