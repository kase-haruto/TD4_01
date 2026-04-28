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

    float toonDiffuse = step(0.0f, rawNdotL); 

    float3 shadowColor = albedo * 0.4f;

    float3 baseDiffuse = lerp(shadowColor, albedo, toonDiffuse);
    diffuse = baseDiffuse * gDirectionalLight.color.rgb * gDirectionalLight.intensity;

    float3 H    = normalize(L + toEye);
    float NdotH = saturate(dot(normal, H));
    
    float toonSpecular = step(0.98f, NdotH); 
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
    
    float toonDiffuse = step(0.0f, rawNdotL);

    float3 shadowColor = albedo * 0.4f;
    float3 baseDiffuse = lerp(shadowColor, albedo, toonDiffuse);
    
    diffuse = baseDiffuse * gPointLight.color.rgb * gPointLight.intensity * attenuation;

    float3 halfVec = normalize(-lightDir + toEye);
    float  NdotH   = saturate(dot(normal, halfVec));
    float toonSpecular = step(0.98f, NdotH);

    specular = gPointLight.color.rgb * toonSpecular * gPointLight.intensity * attenuation;
}
