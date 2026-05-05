float ToonBand(float value, float threshold, float softness) {
    float width = max(softness, 0.0001f);
    return smoothstep(threshold - width, threshold + width, value);
}

float3 EvaluateToonRamp(float ndotl, float3 albedo) {
    float t1 = min(gMaterial.toonThreshold1, min(gMaterial.toonThreshold2, gMaterial.toonThreshold3));
    float t3 = max(gMaterial.toonThreshold1, max(gMaterial.toonThreshold2, gMaterial.toonThreshold3));
    float t2 = gMaterial.toonThreshold1 + gMaterial.toonThreshold2 + gMaterial.toonThreshold3 - t1 - t3;
    float softness = gMaterial.toonEdgeSoftness;

    float3 shadow = albedo * gMaterial.toonShadowColor.rgb;
    float3 midShadow = albedo * gMaterial.toonMidShadowColor.rgb;
    float3 base = albedo * gMaterial.toonBaseColor.rgb;
    float3 highlight = albedo * gMaterial.toonHighlightColor.rgb;

    float3 ramp = lerp(shadow, midShadow, ToonBand(ndotl, t1, softness));
    ramp = lerp(ramp, base, ToonBand(ndotl, t2, softness));
    ramp = lerp(ramp, highlight, ToonBand(ndotl, t3, softness));
    return ramp;
}

float EvaluateToonSpecular(float ndoth) {
    return ToonBand(ndoth, saturate(gMaterial.toonSpecularThreshold), gMaterial.toonSpecularSoftness) *
           max(gMaterial.toonSpecularIntensity, 0.0f);
}

void ComputeToonDirectionalLight(
    float3 normal,
    float3 toEye,
    float3 albedo,
    out float3 diffuse,
    out float3 specular
) {
    diffuse  = 0.0f;
    specular = 0.0f;

    float3 L = -gDirectionalLight.direction;
    float rawNdotL = dot(normal, L);
    float3 rampDiffuse = EvaluateToonRamp(rawNdotL, albedo);
    diffuse = rampDiffuse * gDirectionalLight.color.rgb * gDirectionalLight.intensity;

    float3 H    = normalize(L + toEye);
    float NdotH = saturate(dot(normal, H));
    
    float toonSpecular = EvaluateToonSpecular(NdotH);
    specular = gDirectionalLight.color.rgb * toonSpecular * gDirectionalLight.intensity;
}

void ComputeToonPointLight(
    float3 normal,
    float3 toEye,
    float3 worldPos,
    float3 albedo,
    out float3 diffuse,
    out float3 specular
) {
    diffuse  = 0.0f;
    specular = 0.0f;

    float3 lightDir    = normalize(worldPos - gPointLight.position);
    float  distance    = length(gPointLight.position - worldPos);
    float  attenuation = pow(saturate(1.0f - distance / gPointLight.radius), gPointLight.decay);

    float rawNdotL = dot(normal, -lightDir);

    float3 baseDiffuse = EvaluateToonRamp(rawNdotL, albedo);
    
    diffuse = baseDiffuse * gPointLight.color.rgb * gPointLight.intensity * attenuation;

    float3 halfVec = normalize(-lightDir + toEye);
    float  NdotH   = saturate(dot(normal, halfVec));
    float toonSpecular = EvaluateToonSpecular(NdotH);

    specular = gPointLight.color.rgb * toonSpecular * gPointLight.intensity * attenuation;
}
