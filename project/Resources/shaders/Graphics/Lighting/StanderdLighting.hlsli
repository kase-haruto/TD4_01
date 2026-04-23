#pragma once
#include "../Material/MaterialTypes.hlsli"
#include "LightTypes.hlsli"
#include "LightingTypes.hlsli"

DirectionalLightResult EvaluateDirectionalLight(
    Material material,
    DirectionalLight light,
    SurfaceData surface
) {
	DirectionalLightResult result;
	result.diffuse = 0.0f;
	result.specular = 0.0f;

	float3 L = normalize(-light.direction);
	float rawNdotL = dot(surface.normalWS, L);
	float NdotL = saturate(rawNdotL);

	if (material.enableLighting == 0) {
		float halfLambert = pow(rawNdotL * 0.5f + 0.5f, 2.0f);
		result.diffuse = surface.albedo * light.color.rgb * halfLambert * light.intensity;
	}
	else if (material.enableLighting == 1) {
		result.diffuse = surface.albedo * light.color.rgb * NdotL * light.intensity;
	}

	float3 H = normalize(L + surface.viewDirWS);
	float NdotH = saturate(dot(surface.normalWS, H));
	result.specular = light.color.rgb * pow(NdotH, max(surface.shininess, 1.0f)) * light.intensity;

	return result;
}

PointLightResult EvaluatePointLight(
    Material material,
    PointLight light,
    SurfaceData surface
) {
	PointLightResult result;
	result.diffuse = 0.0f;
	result.specular = 0.0f;

	float3 lightVec = light.position - surface.worldPos;
	float distance = length(lightVec);

	if (distance <= 0.0001f || light.radius <= 0.0001f) {
		return result;
	}

	float3 L = lightVec / distance;
	float attenuation = pow(saturate(1.0f - distance / light.radius), light.decay);

	float NdotL = saturate(dot(surface.normalWS, L));
	result.diffuse = surface.albedo * light.color.rgb * NdotL * light.intensity * attenuation;

	float3 H = normalize(L + surface.viewDirWS);
	float NdotH = saturate(dot(surface.normalWS, H));
	result.specular = light.color.rgb * pow(NdotH, max(surface.shininess, 1.0f)) * light.intensity * attenuation;

	return result;
}