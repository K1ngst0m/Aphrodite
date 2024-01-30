#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec4  outPosition;
layout (location = 1) out vec4  outNormal;
layout (location = 2) out vec4  outAlbedo;
layout (location = 3) out vec4  outMetallicRoughnessAO;
layout (location = 4) out vec4  outEmissive;

layout (set = 0, binding = 4) uniform texture2D textures[5];
struct Material{
    vec4 emissiveFactor;
    vec4 baseColorFactor;
    float alphaCutoff;
    float metallicFactor;
    float roughnessFactor;
    int baseColorId;
    int normalId;
    int occlusionId;
    int emissiveId;
    int metallicRoughnessId;
    int specularGlossinessId;
};
layout (set = 0, binding = 5) uniform MaterialUB{
    Material materials[100];
};
layout (set = 1, binding = 0) uniform sampler samp;
layout (set = 1, binding = 1) uniform sampler shadowSamp;
layout (set = 1, binding = 2) uniform sampler cubemapSamp;

layout( push_constant ) uniform constants
{
    uint objId;
    uint matId;
};

Material mat = materials[matId];

vec3 getNormal()
{
    vec3 normal = mat.normalId > -1 ? texture(sampler2D(textures[mat.normalId], samp), inUV).rgb : inNormal;
    vec3 Q1 = dFdx(inWorldPos);
    vec3 Q2 = dFdy(inWorldPos);
    vec2 st1 = dFdx(inUV);
    vec2 st2 = dFdy(inUV);

    vec3 N = normalize(inNormal);
    vec3 T = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * normalize(normal * 2.0 - 1.0));
}

void main()
{
    float metallic = mat.metallicRoughnessId > -1 ? texture(sampler2D(textures[mat.metallicRoughnessId], samp), inUV).r : mat.metallicFactor;
    float roughness = mat.metallicRoughnessId > -1 ? texture(sampler2D(textures[mat.metallicRoughnessId], samp), inUV).g : mat.roughnessFactor;
    float ao = 1.0f;
    if (mat.occlusionId > -1)
    {
        if (mat.occlusionId == mat.metallicRoughnessId)
        {
            ao = texture(sampler2D(textures[mat.occlusionId], samp), inUV).a;
        }
        else
        {
            ao = texture(sampler2D(textures[mat.occlusionId], samp), inUV).r;
        }
    }

    outPosition = vec4(inWorldPos, 1.0);
    outNormal = vec4(getNormal(), 1.0);
    outAlbedo = mat.baseColorId > -1
        ? texture(sampler2D(textures[mat.baseColorId], samp), inUV)
        : mat.baseColorFactor;

    outMetallicRoughnessAO = vec4(1.0f);
    outMetallicRoughnessAO.x = metallic;
    outMetallicRoughnessAO.y = roughness;
    outMetallicRoughnessAO.z = ao;

    outEmissive = mat.emissiveId > -1 ? texture(sampler2D(textures[mat.emissiveId], samp), inUV) : mat.emissiveFactor;
}
