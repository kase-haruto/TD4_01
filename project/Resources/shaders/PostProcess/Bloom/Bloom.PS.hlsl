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
	float bloomRadius = max(radius, 0.0f);
	if(bloomRadius <= 0.001f) {
		return ExtractBloom(gBloomMask.Sample(gSampler, uv).rgb);
	}

	// Keep bloom in a single fullscreen pass, but avoid the previous 363 taps/pixel
	// Gaussian grid. Linear filtering plus a compact radial kernel gives a similar
	// soft spread at a fraction of the texture bandwidth.
	float2 nearStep = texelSize * bloomRadius * 2.0f;
	float2 midStep = texelSize * bloomRadius * 5.0f;
	float2 farStep = texelSize * bloomRadius * 10.0f;

	float3 bloom = ExtractBloom(gBloomMask.Sample(gSampler, uv).rgb) * 0.18f;

	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2( nearStep.x, 0.0f)).rgb) * 0.08f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(-nearStep.x, 0.0f)).rgb) * 0.08f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(0.0f,  nearStep.y)).rgb) * 0.08f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(0.0f, -nearStep.y)).rgb) * 0.08f;

	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2( midStep.x,  midStep.y)).rgb) * 0.07f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(-midStep.x,  midStep.y)).rgb) * 0.07f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2( midStep.x, -midStep.y)).rgb) * 0.07f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(-midStep.x, -midStep.y)).rgb) * 0.07f;

	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2( farStep.x, 0.0f)).rgb) * 0.06f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(-farStep.x, 0.0f)).rgb) * 0.06f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(0.0f,  farStep.y)).rgb) * 0.06f;
	bloom += ExtractBloom(gBloomMask.Sample(gSampler, uv + float2(0.0f, -farStep.y)).rgb) * 0.06f;

	return bloom;
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	uint width;
	uint height;
	gBloomMask.GetDimensions(width, height);
	float2 texelSize = 1.0f / float2(max(width, 1), max(height, 1));

	float4 baseColor = gSceneColor.Sample(gSampler, input.texcoord);
	if(intensity <= 0.001f) {
		output.color = baseColor;
		return output;
	}

	float3 bloom = SampleBloom(input.texcoord, texelSize) * tint * intensity;

	output.color = float4(saturate(baseColor.rgb + bloom), baseColor.a);
	return output;
}
