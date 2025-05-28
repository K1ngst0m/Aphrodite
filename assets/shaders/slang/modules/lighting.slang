// lighting.slang - Lighting calculation utilities

import math;
import pbr;

namespace lighting
{
    // Common light types
    enum LightType
    {
        eDirectional = 0,
        ePoint = 1,
        eSpot = 2,
        eArea = 3
    };
    
    // Light attenuation calculation
    float calculateAttenuation(float distance, float falloffStart, float falloffEnd)
    {
        // Linear falloff between start and end distances
        return math::saturate((falloffEnd - distance) / (falloffEnd - falloffStart));
    }
    
    // Point light attenuation
    float calculatePointLightAttenuation(float distance, float radius)
    {
        // Physically-based inverse square falloff
        float att = 1.0 / max(distance * distance, 0.01);
        
        // Attenuate to zero at radius
        float falloffFactor = math::saturate(1.0 - pow(distance / radius, 4.0));
        falloffFactor *= falloffFactor;
        
        return att * falloffFactor;
    }
    
    // Spot light calculation with cone attenuation
    float calculateSpotLight(float3 L, float3 lightDir, float innerConeAngle, float outerConeAngle)
    {
        float cosAngle = dot(-L, lightDir);
        float innerCone = cos(innerConeAngle);
        float outerCone = cos(outerConeAngle);
        
        // Smooth attenuation between inner and outer cone angles
        return math::smoothstep(outerCone, innerCone, cosAngle);
    }
    
    // Calculate lighting contribution from a directional light
    float3 calculateDirectionalLight(float3 normal, float3 viewDir, float3 lightDir, 
                                    float3 lightColor, float3 albedo, 
                                    float roughness, float metalness)
    {
        float3 L = -lightDir; // Light direction (pointing from surface to light)
        
        // Calculate PBR lighting using Cook-Torrance BRDF
        return pbr::cookTorranceBRDF(normal, viewDir, L, albedo, roughness, metalness) * lightColor;
    }
    
    // Calculate lighting contribution from a point light
    float3 calculatePointLight(float3 normal, float3 viewDir, float3 worldPos,
                              float3 lightPos, float3 lightColor, float lightRadius,
                              float3 albedo, float roughness, float metalness)
    {
        // Calculate light vector and distance
        float3 L = lightPos - worldPos;
        float distance = length(L);
        L = normalize(L);
        
        // Calculate attenuation
        float attenuation = calculatePointLightAttenuation(distance, lightRadius);
        
        // Calculate PBR lighting using Cook-Torrance BRDF
        float3 radiance = lightColor * attenuation;
        return pbr::cookTorranceBRDF(normal, viewDir, L, albedo, roughness, metalness) * radiance;
    }
    
    // Calculate lighting contribution from a spot light
    float3 calculateSpotLight(float3 normal, float3 viewDir, float3 worldPos,
                             float3 lightPos, float3 lightDir, float3 lightColor,
                             float lightRadius, float innerConeAngle, float outerConeAngle,
                             float3 albedo, float roughness, float metalness)
    {
        // Calculate light vector and distance
        float3 L = lightPos - worldPos;
        float distance = length(L);
        L = normalize(L);
        
        // Calculate spot cone attenuation
        float spotFactor = calculateSpotLight(L, lightDir, innerConeAngle, outerConeAngle);
        
        // Calculate distance attenuation
        float attenuation = calculatePointLightAttenuation(distance, lightRadius);
        
        // Combined attenuation
        float finalAttenuation = attenuation * spotFactor;
        
        // Calculate PBR lighting using Cook-Torrance BRDF
        float3 radiance = lightColor * finalAttenuation;
        return pbr::cookTorranceBRDF(normal, viewDir, L, albedo, roughness, metalness) * radiance;
    }
    
    // Calculate ambient occlusion factor
    float3 calculateAmbientOcclusion(float3 ambient, float aoFactor)
    {
        return ambient * aoFactor;
    }
    
    // Simple ambient lighting calculation
    float3 calculateAmbient(float3 albedo, float3 ambientColor, float ambientIntensity)
    {
        return albedo * ambientColor * ambientIntensity;
    }
    
    // Calculate Image-Based Lighting contribution
    float3 calculateIBL(float3 N, float3 V, float3 albedo, float roughness, float metalness,
                      Texture2D irradianceMap, Texture2D prefilteredEnvMap, 
                      Texture2D brdfLUT, SamplerState envSampler)
    {
        // Calculate reflection vector
        float3 R = reflect(-V, N);
        
        // Calculate F0 (Fresnel reflection at normal incidence)
        float3 F0 = pbr::calculateF0(albedo, metalness);
        
        // Sample irradiance map for diffuse IBL
        float3 irradiance = irradianceMap.Sample(envSampler, N).rgb;
        
        // Sample prefiltered environment map for specular IBL
        float mipLevel = roughness * 5.0; // Assuming 6 mip levels (0-5)
        float3 prefilteredColor = prefilteredEnvMap.SampleLevel(envSampler, R, mipLevel).rgb;
        
        // Sample BRDF LUT
        float NdotV = math::saturate(dot(N, V));
        float2 brdf = brdfLUT.Sample(envSampler, float2(NdotV, roughness)).rg;
        
        // Fresnel with roughness consideration for IBL
        float3 F = pbr::fresnelSchlickRoughness(NdotV, F0, roughness);
        
        // Diffuse and specular IBL contributions
        float3 kS = F;
        float3 kD = (1.0 - kS) * (1.0 - metalness);
        
        float3 diffuseIBL = irradiance * albedo;
        float3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);
        
        // Combined IBL lighting
        return kD * diffuseIBL + specularIBL;
    }
} 