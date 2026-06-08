#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

void main(VertexShaderOutput input) {
	// 不透明なマスクテクセルだけがステンシルを書き込む。色書き込みは PSO 側で無効化している。
	float alpha = gTexture.Sample(gSampler, input.texcoord).a;
	if(alpha <= 0.5f) {
		discard;
	}
}
