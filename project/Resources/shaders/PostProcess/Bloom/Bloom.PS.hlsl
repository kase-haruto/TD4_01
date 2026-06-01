#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
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
	if(threshold < 0.0f) {
		return color;
	}

	float brightness = Luminance(color);
	float knee = max(softKnee, 0.0001f);
	float soft = saturate((brightness - threshold + knee) / (2.0f * knee));
	float contribution = max(brightness - threshold, 0.0f) + soft * soft * knee;
	contribution /= max(brightness, 0.0001f);
	return color * saturate(contribution);
}

float3 SampleBloom(float2 uv, float2 texelSize) {
	static const float2 offsets[12] = {
		float2( 1.0f,  0.0f), float2(-1.0f,  0.0f),
		float2( 0.0f,  1.0f), float2( 0.0f, -1.0f),
		float2( 1.0f,  1.0f), float2(-1.0f,  1.0f),
		float2( 1.0f, -1.0f), float2(-1.0f, -1.0f),
		float2( 2.0f,  0.0f), float2(-2.0f,  0.0f),
		float2( 0.0f,  2.0f), float2( 0.0f, -2.0f)
	};
	static const float weights[12] = {
		0.085f, 0.085f, 0.085f, 0.085f,
		0.060f, 0.060f, 0.060f, 0.060f,
		0.040f, 0.040f, 0.040f, 0.040f
	};

	float spread = max(radius, 0.0f) * 2.0f;
	float3 bloom = ExtractBloom(gTexture.Sample(gSampler, uv).rgb) * 0.24f;
	float totalWeight = 0.24f;

	for(int ring = 1; ring <= 3; ++ring) {
		float ringScale = spread * ring;
		for(int i = 0; i < 12; ++i) {
			float2 sampleUV = uv + offsets[i] * texelSize * ringScale;
			float weight = weights[i] / ring;
			bloom += ExtractBloom(gTexture.Sample(gSampler, sampleUV).rgb) * weight;
			totalWeight += weight;
		}
	}

	return bloom / max(totalWeight, 0.0001f);
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	uint width;
	uint height;
	gTexture.GetDimensions(width, height);
	float2 texelSize = 1.0f / float2(max(width, 1), max(height, 1));

	float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
	float3 bloom = SampleBloom(input.texcoord, texelSize) * tint * intensity;

	output.color = float4(saturate(baseColor.rgb + bloom), baseColor.a);
	return output;
}
