// material.slang - Material parameter standardization

import bindless;

// Standard material parameter structure that integrates with material system
struct StandardMaterial
{
    // PBR Base Parameters
    float4 baseColor;
    float roughness;
    float metallic;
    float normalScale;
    float ao;
    
    // Emission Parameters
    float3 emissiveColor;
    float emissiveIntensity;
    
    // Advanced Parameters
    float specularScale;
    float clearCoat;
    float clearCoatRoughness;
    float alpha;
    
    // Special Flags
    uint flags;
};

// Standard material textures
struct MaterialTextures
{
    bindless::Texture albedoMap;
    bindless::Texture normalMap;
    bindless::Texture metallicRoughnessMap; // R=Metallic, G=Roughness
    bindless::Texture aoMap;
    bindless::Texture emissiveMap;
    bindless::Texture specularMap;
    bindless::Texture displacementMap;
    
    bindless::Sampler defaultSampler;
    bindless::Sampler clampedSampler;
};

// Material feature flags
enum MaterialFlags : uint
{
    USE_ALPHA_TEST        = 1 << 0,
    USE_ALPHA_BLEND       = 1 << 1,
    USE_NORMAL_MAP        = 1 << 2,
    USE_METALROUGHNESS_MAP = 1 << 3,
    USE_AO_MAP            = 1 << 4,
    USE_EMISSIVE_MAP      = 1 << 5,
    USE_DISPLACEMENT_MAP  = 1 << 6,
    USE_CLEAR_COAT        = 1 << 7,
    USE_ANISOTROPY        = 1 << 8,
    USE_SUBSURFACE        = 1 << 9,
    USE_TRANSMISSION      = 1 << 10
};

// Material system API namespace
namespace material
{
    // Check if a material has a specific feature flag
    bool hasFeature(uint materialFlags, MaterialFlags flag)
    {
        return (materialFlags & flag) != 0;
    }
    
    // Sample a material texture with proper defaults
    float4 sampleTexture(bindless::Texture tex, bindless::Sampler samp, float2 uv, float4 defaultValue)
    {
        // Using a default white texture if tex.id is invalid
        return tex.id != 0xFFFFFFFF ? tex.sample(samp, uv) : defaultValue;
    }
    
    // Sample albedo with proper defaults
    float4 sampleAlbedo(MaterialTextures textures, float2 uv, float4 baseColor)
    {
        return sampleTexture(textures.albedoMap, textures.defaultSampler, uv, baseColor);
    }
    
    // Sample normal map and convert to normal vector
    float3 sampleNormal(MaterialTextures textures, float2 uv, float normalScale)
    {
        float4 normalSample = sampleTexture(textures.normalMap, textures.defaultSampler, uv, float4(0.5, 0.5, 1.0, 1.0));
        float3 N = normalize(normalSample.rgb * 2.0 - 1.0);
        N.xy *= normalScale;
        return normalize(N);
    }
    
    // Sample metallic and roughness values
    float2 sampleMetallicRoughness(MaterialTextures textures, float2 uv, float metallicParam, float roughnessParam)
    {
        float4 mrSample = sampleTexture(textures.metallicRoughnessMap, textures.defaultSampler, uv, float4(0.0, 1.0, 0.0, 1.0));
        float metallic = mrSample.r * metallicParam;
        float roughness = mrSample.g * roughnessParam;
        return float2(metallic, roughness);
    }
    
    // Sample ambient occlusion
    float sampleAO(MaterialTextures textures, float2 uv, float aoParam)
    {
        float4 aoSample = sampleTexture(textures.aoMap, textures.defaultSampler, uv, float4(1.0, 1.0, 1.0, 1.0));
        return aoSample.r * aoParam;
    }
    
    // Sample emissive color
    float3 sampleEmissive(MaterialTextures textures, float2 uv, float3 emissiveColor, float intensity)
    {
        float4 emissiveSample = sampleTexture(textures.emissiveMap, textures.defaultSampler, uv, float4(0.0, 0.0, 0.0, 1.0));
        return emissiveSample.rgb * emissiveColor * intensity;
    }
    
    // Get material data with proper defaults and texture sampling
    void prepareMaterialData(inout StandardMaterial material, MaterialTextures textures, float2 uv, 
                            out float3 albedo, out float metallic, out float roughness, 
                            out float3 normal, out float ao, out float3 emissive)
    {
        // Sample albedo with base color as default
        float4 albedoSample = sampleAlbedo(textures, uv, material.baseColor);
        albedo = albedoSample.rgb;
        
        // Sample metallic/roughness if available
        if (hasFeature(material.flags, MaterialFlags::USE_METALROUGHNESS_MAP))
        {
            float2 mr = sampleMetallicRoughness(textures, uv, material.metallic, material.roughness);
            metallic = mr.x;
            roughness = mr.y;
        }
        else
        {
            metallic = material.metallic;
            roughness = material.roughness;
        }
        
        // Ensure roughness is never completely smooth (avoid divide by zero in PBR calculations)
        roughness = max(roughness, 0.05);
        
        // Sample normal map if available
        if (hasFeature(material.flags, MaterialFlags::USE_NORMAL_MAP))
        {
            normal = sampleNormal(textures, uv, material.normalScale);
        }
        else
        {
            normal = float3(0, 0, 1); // Default normal in tangent space
        }
        
        // Sample ambient occlusion if available
        if (hasFeature(material.flags, MaterialFlags::USE_AO_MAP))
        {
            ao = sampleAO(textures, uv, material.ao);
        }
        else
        {
            ao = material.ao;
        }
        
        // Sample emissive map if available
        if (hasFeature(material.flags, MaterialFlags::USE_EMISSIVE_MAP))
        {
            emissive = sampleEmissive(textures, uv, material.emissiveColor, material.emissiveIntensity);
        }
        else
        {
            emissive = material.emissiveColor * material.emissiveIntensity;
        }
    }
} 