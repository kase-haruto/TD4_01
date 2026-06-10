#include "Object2d.hlsli"

struct PSOutput {
	float4 color : SV_TARGET0;
};

struct Material {
	float4   color;
	float4x4 uvTransform;

	float2 uvOffset;
	float2 uvScale;

	float  fillAmount;
	float2 fillOrigin;
	int    fillMethod;
};

ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gTexture : register(t0);
SamplerState      gSampler : register(s0);

PSOutput main(VSOutput input) {
	PSOutput output;

	//--------------------------------
	// baseUV（0〜1）はそのまま保持
	//--------------------------------
	float2 baseUV = input.texcoord;

	//--------------------------------
	// フィル判定
	//--------------------------------
	if (gMaterial.fillMethod == 1) {
		// 横方向フィル
		if (gMaterial.fillOrigin.x < 0.5f) {
			if (baseUV.x > gMaterial.fillAmount) discard;
		} else {
			if (baseUV.x < 1.0f - gMaterial.fillAmount) discard;
		}
	} else if (gMaterial.fillMethod == 2) {
		// 縦方向フィル（baseUV.y は 0=上, 1=下）
		// fillOrigin.y < 0.5 : 下が原点（下を残す＝上から削れる）
		// fillOrigin.y >=0.5 : 上が原点（上を残す＝下から削れる）
		if (gMaterial.fillOrigin.y < 0.5f) {
			if (baseUV.y < 1.0f - gMaterial.fillAmount) discard;
		} else {
			if (baseUV.y > gMaterial.fillAmount) discard;
		}
	}

	//--------------------------------
	// アニメーション用 UV 計算
	//--------------------------------

	// 分割スケール適用
	float2 animUV = baseUV * gMaterial.uvScale;

	// 分割オフセット適用
	animUV += gMaterial.uvOffset;

	// UV Transform（回転・移動）
	float4 uvT = mul(float4(animUV, 0, 1), gMaterial.uvTransform);
	animUV = uvT.xy;

	//--------------------------------
	// テクスチャサンプリング
	//--------------------------------
	float4 texColor = gTexture.Sample(gSampler, animUV);

	output.color = texColor * gMaterial.color;
	return output;
}