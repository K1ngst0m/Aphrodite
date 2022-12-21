#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (std140, set = 0, binding = 0) uniform SceneInfoUB{
    vec4 ambientColor;
    int cameraCount;
    int lightCount;
};

layout (set = 0, binding = 1) uniform CameraUB{
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} cameraData[];

layout (set = 0, binding = 2) uniform LightUB{
    vec3 color;
    vec3 position;
    vec3 direction;
} lightData[];

const float PI = 3.14159265359;

layout(set = 1, binding = 0) uniform ObjectUB{
    mat4 matrix;
};

layout(set = 2, binding = 0) uniform MatInfoUB{
    vec4 emissiveFactor;
    vec4 baseColorFactor;
    float     alphaCutoff;
    float     metallicFactor;
    float     roughnessFactor;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
    int metallicRoughnessTextureIndex;
    int specularGlossinessTextureIndex;
} ;
layout(set = 2, binding = 1) uniform texture2D colorMap;
layout(set = 2, binding = 2) uniform texture2D normalMap;
layout(set = 2, binding = 3) uniform texture2D physicalDescMap;
layout(set = 2, binding = 4) uniform texture2D aoMap;
layout(set = 2, binding = 5) uniform texture2D emissiveMap;
layout(set = 3, binding = 0) uniform sampler samp;

vec3 getNormal()
{
    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent.xyz);
    vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
    mat3 TBN = mat3(T, B, N);
    return TBN * normalize(texture(sampler2D(normalMap, samp), inUV).xyz * 2.0 - vec3(1.0));
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = baseColorTextureIndex > -1 ? texture(sampler2D(colorMap, samp), inUV).rgb : baseColorFactor.xyz;
    float metallic = metallicRoughnessTextureIndex > -1 ? texture(sampler2D(physicalDescMap, samp), inUV).r : metallicFactor;
    float roughness = metallicRoughnessTextureIndex > -1 ? texture(sampler2D(physicalDescMap, samp), inUV).g : roughnessFactor;
    vec3 ao = vec3(1.0f);
    if (occlusionTextureIndex > -1){
        if (occlusionTextureIndex == metallicRoughnessTextureIndex){
            ao = texture(sampler2D(aoMap, samp), inUV).aaa;
        }
        else{
            ao = texture(sampler2D(aoMap, samp), inUV).rgb;
        }
    }
    vec3 emissive = emissiveTextureIndex > -1 ? texture(sampler2D(emissiveMap, samp), inUV).rgb : emissiveFactor.xyz;

    vec3 N = getNormal();
    vec3 V = normalize(cameraData[0].viewPos - inWorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0f);

    for (int i = 0; i < lightCount; i++)
    {
        vec3 L = normalize(-lightData[0].direction);
        vec3 H = normalize(V + L);
        vec3 R = reflect(L, N);
        vec3 radiance = vec3(3.0f).xyz;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(clamp(dot(H, V), 0.0f, 1.0f), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;

        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0f);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = ambientColor.xyz * albedo * ao;
    vec3 color = emissive + Lo + ambient;

    outColor = vec4(color, 1.0f);
}
