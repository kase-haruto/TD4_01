#pragma once

float3 ApplyToneMappingAndGamma(float3 color, float exposure) {
	float3 toneMapped = color * exposure / (color * exposure + 1.0f);
	return pow(toneMapped, 1.0f / 2.2f);
}