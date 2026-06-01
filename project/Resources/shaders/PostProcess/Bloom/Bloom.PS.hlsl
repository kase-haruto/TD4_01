#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gSceneColor : register(t0);
Texture2D<float4> gBloomMask : register(t1);
SamplerState gSampler : register(s0);

cbuffer BloomParameter : register(b0) {
	float intensity;
	float threshold;
	float softKnee;
	float radius;
	float3 tint;
	float padding;
}

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

float Luminance(float3 color) {
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ExtractBloom(float3 color) {
	// The mask already contains the emissive contribution, 
	// so we don't need to perform additional luminance thresholding here.
	return color;
}

float3 SampleBloom(float2 uv, float2 texelSize) {
	// Increased spread for more blur
	float spread = max(radius, 0.0f) * 4.0f;
	float3 bloom = float3(0,0,0);
	float totalWeight = 0.0f;

	// Use a standard Gaussian-like distribution
	const int samples = 5;
	for (int x = -samples; x <= samples; ++x) {
		for (int y = -samples; y <= samples; ++y) {
			float2 offset = float2(float(x), float(y));
			float2 sampleUV = uv + offset * texelSize * spread;
			
			float weight = exp(-(dot(offset, offset) / (2.0f * 2.0f)));
			
			bloom += ExtractBloom(gBloomMask.Sample(gSampler, sampleUV).rgb) * weight;
			totalWeight += weight;
		}
	}

	return bloom / max(totalWeight, 0.0001f);
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	uint width;
	uint height;
	gBloomMask.GetDimensions(width, height);
	float2 texelSize = 1.0f / float2(max(width, 1), max(height, 1));

	float4 baseColor = gSceneColor.Sample(gSampler, input.texcoord);
	float3 bloom = SampleBloom(input.texcoord, texelSize) * tint * intensity;

	output.color = float4(saturate(baseColor.rgb + bloom), baseColor.a);
	return output;
}
