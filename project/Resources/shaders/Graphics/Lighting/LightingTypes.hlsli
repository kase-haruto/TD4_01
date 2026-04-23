#pragma once

//data

struct SurfaceData {
	float3 albedo;
	float3 normalWS;
	float3 viewDirWS;
	float3 worldPos;
	float alpha;
	float shininess;
};

struct DirectionalLightResult {
	float3 diffuse;
	float3 specular;
};

struct PointLightResult {
	float3 diffuse;
	float3 specular;
};