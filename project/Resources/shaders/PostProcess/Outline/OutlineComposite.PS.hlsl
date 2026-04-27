#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gColor : register(t0);
Texture2D<float> gDepth : register(t1);
Texture2D<float4> gNormal : register(t2);
SamplerState gSampler : register(s0);

cbuffer OutlineCompositeConstants : register(b0) {
	float4 outlineColor;
	float2 texelSize;
	float depthThreshold;
	float normalThreshold;
	float depthScale;
	float normalScale;
	float thickness;
	float padding;
}

float NormalEdge(float3 centerNormal, float centerMask, float3 sampleNormal, float sampleMask) {
	float valid = saturate(centerMask + sampleMask);
	float d = length(centerNormal - sampleNormal) * normalScale;
	return smoothstep(normalThreshold, 1.0f, d) * valid;
}

float DepthEdge(float centerDepth, float centerMask, float sampleDepth, float sampleMask) {
	float valid = saturate(centerMask + sampleMask);
	float nearDepth = max(min(centerDepth, sampleDepth), 1e-5f);
	float d = abs(centerDepth - sampleDepth) / nearDepth * depthScale;
	return smoothstep(depthThreshold, 1.0f, d) * valid;
}

float4 main(VertexShaderOutput input) : SV_TARGET {
	const float2 uv = input.texcoord;
	const float4 baseColor = gColor.Sample(gSampler, uv);
	const float centerDepth = gDepth.Sample(gSampler, uv);
	const float4 centerNormalTex = gNormal.Sample(gSampler, uv);
	const float3 centerNormal = centerNormalTex.xyz * 2.0f - 1.0f;
	const float centerMask = centerNormalTex.a;

	float edge = 0.0f;
	const int radius = clamp((int)round(thickness), 1, 4);

	[loop]
	for(int y = -radius; y <= radius; ++y) {
		[loop]
		for(int x = -radius; x <= radius; ++x) {
			if(x == 0 && y == 0) continue;
			float2 o = float2((float)x, (float)y) * texelSize;
			float sd = gDepth.Sample(gSampler, uv + o);
			float4 snTex = gNormal.Sample(gSampler, uv + o);
			float3 sn = snTex.xyz * 2.0f - 1.0f;
			float sm = snTex.a;
			edge = max(edge, DepthEdge(centerDepth, centerMask, sd, sm));
			edge = max(edge, NormalEdge(centerNormal, centerMask, sn, sm));
		}
	}

	edge = saturate(edge);
	return lerp(baseColor, outlineColor, edge * outlineColor.a);
}
