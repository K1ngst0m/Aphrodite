// pbr.slang - Physically Based Rendering utilities

import math;

namespace pbr
{
    // Normal Distribution Function (GGX/Trowbridge-Reitz)
    float distributionGGX(float3 N, float3 H, float roughness)
    {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = math::saturate(dot(N, H));
        float NdotH2 = NdotH * NdotH;
        
        float nom = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = math::PI * denom * denom;
        
        return nom / max(denom, 0.0000001);
    }
    
    // Geometry Function (Smith's method with GGX)
    float geometrySchlickGGX(float NdotV, float roughness)
    {
        float r = (roughness + 1.0);
        float k = (r * r) / 8.0;
        
        float nom = NdotV;
        float denom = NdotV * (1.0 - k) + k;
        
        return nom / max(denom, 0.0000001);
    }
    
    float geometrySmith(float3 N, float3 V, float3 L, float roughness)
    {
        float NdotV = math::saturate(dot(N, V));
        float NdotL = math::saturate(dot(N, L));
        float ggx2 = geometrySchlickGGX(NdotV, roughness);
        float ggx1 = geometrySchlickGGX(NdotL, roughness);
        
        return ggx1 * ggx2;
    }
    
    // Fresnel Function (Schlick's approximation)
    float3 fresnelSchlick(float cosTheta, float3 F0)
    {
        return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
    }
    
    // Fresnel Function with roughness consideration for IBL
    float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
    {
        return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
    }
    
    // Default F0 value (dielectric surface)
    static const float3 F0_DEFAULT = float3(0.04);
    
    // Calculate F0 (Fresnel reflection at normal incidence) based on metalness
    float3 calculateF0(float3 albedo, float metalness)
    {
        return lerp(F0_DEFAULT, albedo, metalness);
    }
    
    // Cook-Torrance BRDF
    float3 cookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metalness)
    {
        float3 H = normalize(V + L);
        
        // Calculate F0 (Fresnel reflection at normal incidence)
        float3 F0 = calculateF0(albedo, metalness);
        
        // Cook-Torrance BRDF terms
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // Calculate specular component
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.0000001);
        
        // Calculate diffuse component (energy conservation)
        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD *= 1.0 - metalness; // No diffuse for metallic surfaces
        
        // Final BRDF combines diffuse and specular
        float NdotL = max(dot(N, L), 0.0);
        return (kD * albedo / math::PI + specular) * NdotL;
    }
    
    // Calculate tangent-space normal from normal map
    float3 calculateNormalFromMap(float3 normalMap, float3 normal, float3 tangent, float3 bitangent, float normalScale)
    {
        // Unpack normal map (assuming it's in tangent space)
        float3 N = normalize(normalMap * 2.0 - 1.0);
        N.xy *= normalScale;
        
        // Build the tangent space matrix
        float3 T = normalize(tangent);
        float3 B = normalize(bitangent);
        float3x3 TBN = float3x3(T, B, normal);
        
        // Transform normal from tangent space to world space
        return normalize(mul(N, TBN));
    }
    
    // Apply gamma correction to linear color
    float3 gammaCorrect(float3 linearColor, float gamma = 2.2)
    {
        return pow(linearColor, float3(1.0 / gamma));
    }
    
    // Apply exposure adjustment
    float3 applyExposure(float3 color, float exposure)
    {
        return float3(1.0) - exp(-color * exposure);
    }
} 