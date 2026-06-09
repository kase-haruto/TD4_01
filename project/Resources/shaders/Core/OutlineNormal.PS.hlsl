#include "Object3D.hlsli"

struct Material {
	float4 color;
	int enableLighting;
	float3 pad;
	float4x4 uvTransform;
};

cbuffer MaterialConstants : register(b9) {
	Material gMaterial;
}

Texture2D<float4> gTexture : register(t9);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET {
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	float alpha = gMaterial.color.a * textureColor.a;
	if(alpha <= 0.01f) {
		discard;
	}

	float3 n = normalize(input.normal);
	return float4(n * 0.5f + 0.5f, input.position.z);
}
