#pragma once

#include "Object3D.hlsli"

#include "../Graphics/Material/MaterialTypes.hlsli"
#include "../Graphics/Lighting/LightTypes.hlsli"
#include "../Graphics/Lighting/LightingTypes.hlsli"
#include "../Graphics/Shadow/ShadowTypes.hlsli"

#include "../Graphics/Lighting/LightingCommon.hlsli"
#include "../Graphics/Lighting/StanderdLighting.hlsli"
#include "../Graphics/Shadow/ShadowCommon.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialConstants : register(b0) {
	Material gMaterial;
}

cbuffer DirectionalLightConstants : register(b2) {
	DirectionalLight gDirectionalLight;
}

cbuffer ShadowConstantsBuffer : register(b3) {
	ShadowParams gShadowConstants;
}

cbuffer PointLightConstants : register(b4) {
	PointLight gPointLight;
}

cbuffer RaytracingShadowParamBuffer : register(b5) {
	RaytracingShadowParams gRtParm;
}

///////////////////////////////////////////////////////////////////////////////
//                            resources
///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentMap : register(t1);
Texture2D<float> gShadowMap : register(t2);
RaytracingAccelerationStructure gRtScene : register(t3);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

///////////////////////////////////////////////////////////////////////////////
//                            出力
///////////////////////////////////////////////////////////////////////////////
struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

///////////////////////////////////////////////////////////////////////////////
//                              main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

	SurfaceData surface;
	surface.albedo = gMaterial.color.rgb * textureColor.rgb;
	surface.normalWS = normalize(input.normal);
	surface.viewDirWS = normalize(cameraPosition - input.worldPosition);
	surface.worldPos = input.worldPosition;
	surface.alpha = gMaterial.color.a * textureColor.a;
	surface.shininess = gMaterial.shiniess;

	if (gMaterial.enableLighting == 4) {
		if (surface.alpha <= 0.01f) {
			discard;
		}

		output.color = float4(surface.albedo, surface.alpha);
		return output;
	}

	DirectionalLightResult directional = EvaluateDirectionalLight(
        gMaterial,
        gDirectionalLight,
        surface
    );

	PointLightResultpoint = EvaluatePointLight(
        gMaterial,
        gPointLight,
        surface
    );

	float3 L = normalize(-gDirectionalLight.direction);

	float shadow = 1.0f;
	if (dot(surface.normalWS, L) > 0.0f) {
		shadow = (gRtParm.gIsSoft != 0)
            ? ComputeDirectionalSoftShadowRT(
                gRtScene,
                gRtParm,
                surface.worldPos,
                surface.normalWS,
                L
            )
            : ComputeDirectionalHardShadowRT(
                gRtScene,
                gRtParm,
                surface.worldPos,
                surface.normalWS,
                L
            );
	}

	directional.diffuse *= shadow;
	directional.specular *= shadow;

	float3 litColor = 0.0f;
	litColor += directional.diffuse;
	litColor += directional.specular;
	litColor += point.
	diffuse;
    litColor += point.
	specular;

	float3 ambient = surface.albedo * 0.07f;
	litColor += ambient;

	if (gMaterial.isReflect != 0) {
		float3 viewDirFromCamera = normalize(input.worldPosition - cameraPosition);
		float3 reflectDir = reflect(viewDirFromCamera, surface.normalWS);

		const float maxMipLevel = 7.0f;
		float mipLevel = saturate(gMaterial.roughness) * maxMipLevel;

		float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
		litColor += envColor * gMaterial.environmentCoefficient;
	}

	float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

	if (surface.alpha <= 0.01f) {
		discard;
	}

	output.color = float4(finalColor, surface.alpha);
	return output;
}