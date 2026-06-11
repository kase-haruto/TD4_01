#include "../Copy/CopyImage.hlsli"

// ポストエフェクトへ入力されるシーンカラー
Texture2D<float4> gSceneColor : register(t0);

// メイン3D描画で作成された深度テクスチャ
// D3Dの深度は非線形なので、そのままでは手前側の調整が極端に難しい
Texture2D<float> gSceneDepth : register(t1);

SamplerState gSampler : register(s0);

// 被写界深度用パラメータ
cbuffer DepthOfFieldParameter : register(b0) {
	// ピントが合う深度
	float focusDepth;

	// ピントが合って見える深度幅
	float focusRange;

	// 最大ぼかし半径。ピクセル単位
	float maxBlurRadius;

	// ぼかしのかかり具合
	float intensity;

	// 深度をリニア化するときのNear平面
	float nearPlane;

	// 深度をリニア化するときのFar平面
	float farPlane;

	// 焦点より手前をぼかす強さ
	// 0.0なら背景だけをぼかし、1.0なら手前と奥を同じようにぼかす
	float foregroundBlur;

	float padding;
}

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

// D3Dの非線形深度をビュー空間距離へ戻す
float LinearizeDepth(float rawDepth) {
	float nearZ = max(nearPlane, 0.0001f);
	float farZ = max(farPlane, nearZ + 0.0001f);

	// D3D 0..1 深度から正のビュー空間Zを復元する
	float viewZ = (nearZ * farZ) / max(farZ - rawDepth * (farZ - nearZ), 0.0001f);

	// UIで扱いやすいよう、Near～Farを0.0～1.0へ正規化する
	return saturate((viewZ - nearZ) / max(farZ - nearZ, 0.0001f));
}

// リニア深度差から 0.0 ～ 1.0 のぼかし率を計算する
float CalcBlurAmount(float linearDepth) {
	float range = max(focusRange, 0.0001f);
	float focusDiff = linearDepth - focusDepth;

	// 焦点より奥は通常どおりぼかす
	float backgroundBlur = saturate((focusDiff - range) / range);

	// 焦点より手前は foregroundBlur で抑制する
	// 背景ぼかし用途では0.0にして、手前の主役がぼけないようにする
	float frontBlur = saturate((-focusDiff - range) / range) * saturate(foregroundBlur);

	float blur = max(backgroundBlur, frontBlur);
	return saturate(blur * max(intensity, 0.0f));
}

// カラーを1点サンプリングする
float3 SampleColor(float2 uv) {
	return gSceneColor.Sample(gSampler, uv).rgb;
}

// 深度差に応じた疑似円形ブラーを計算する
float3 SampleDepthOfField(float2 uv, float2 texelSize, float blurAmount) {
	float radius = max(maxBlurRadius, 0.0f) * blurAmount;

	// ぼかし量が小さい場合は元色をそのまま使い、余計なサンプリングを避ける
	if(radius <= 0.001f) {
		return SampleColor(uv);
	}

	// 中心色をやや強めに残すことで、少ないサンプル数でも色が薄くなりすぎないようにする
	float3 color = SampleColor(uv) * 0.20f;
	float weight = 0.20f;

	// 近距離サンプル。輪郭の破綻を抑えつつ、軽いにじみを作る
	float2 nearStep = texelSize * radius * 0.75f;
	color += SampleColor(uv + float2( nearStep.x, 0.0f)) * 0.08f;
	color += SampleColor(uv + float2(-nearStep.x, 0.0f)) * 0.08f;
	color += SampleColor(uv + float2(0.0f,  nearStep.y)) * 0.08f;
	color += SampleColor(uv + float2(0.0f, -nearStep.y)) * 0.08f;
	weight += 0.32f;

	// 中距離サンプル。斜め方向を足して円形に近いぼけ味へ寄せる
	float2 midStep = texelSize * radius * 1.50f;
	color += SampleColor(uv + float2( midStep.x,  midStep.y)) * 0.07f;
	color += SampleColor(uv + float2(-midStep.x,  midStep.y)) * 0.07f;
	color += SampleColor(uv + float2( midStep.x, -midStep.y)) * 0.07f;
	color += SampleColor(uv + float2(-midStep.x, -midStep.y)) * 0.07f;
	weight += 0.28f;

	// 遠距離サンプル。最大半径付近の広いぼけを補う
	float2 farStep = texelSize * radius * 2.40f;
	color += SampleColor(uv + float2( farStep.x, 0.0f)) * 0.05f;
	color += SampleColor(uv + float2(-farStep.x, 0.0f)) * 0.05f;
	color += SampleColor(uv + float2(0.0f,  farStep.y)) * 0.05f;
	color += SampleColor(uv + float2(0.0f, -farStep.y)) * 0.05f;
	weight += 0.20f;

	return color / max(weight, 0.0001f);
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	// 現在の入力サイズから1ピクセル分のUV距離を求める。
	// RTリサイズ後もこの値が自動的に変わるため、C++側でサイズ定数を更新する必要はない。
	uint width;
	uint height;
	gSceneColor.GetDimensions(width, height);
	float2 texelSize = 1.0f / float2(max(width, 1), max(height, 1));

	float rawDepth = gSceneDepth.Sample(gSampler, input.texcoord);
	float linearDepth = LinearizeDepth(rawDepth);
	float blurAmount = CalcBlurAmount(linearDepth);

	float4 baseColor = gSceneColor.Sample(gSampler, input.texcoord);
	float3 dofColor = SampleDepthOfField(input.texcoord, texelSize, blurAmount);

	// blurAmount で元色とぼかし色を補間し、焦点付近をシャープに残す
	output.color = float4(lerp(baseColor.rgb, dofColor, blurAmount), baseColor.a);
	return output;
}
