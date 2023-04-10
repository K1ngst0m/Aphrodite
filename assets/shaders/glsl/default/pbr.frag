#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (std140, set = 0, binding = 0) uniform SceneInfoUB{
    vec4 ambientColor;
    uint cameraCount;
    uint lightCount;
};

struct Camera{
    mat4 view;
    mat4 proj;
    vec3 viewPos;
};

layout (set = 0, binding = 2) uniform CameraUB{
    Camera cameras[100];
};

struct Light{
    vec3 color;
    vec3 position;
    vec3 direction;
};
layout (set = 0, binding = 3) uniform LightUB{
    Light lights[100];
};

layout (set = 0, binding = 4) uniform texture2D textures[];

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
layout (set = 0, binding = 6) uniform textureCube skybox;
layout (set = 1, binding = 0) uniform sampler samp;
layout (set = 1, binding = 1) uniform sampler sampCubeMap;

const float PI = 3.14159265359;

layout( push_constant ) uniform constants
{
    uint objId;
    uint matId;
};

Material mat = materials[matId];

vec3 getNormal()
{
    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent.xyz);
    vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
    mat3 TBN = mat3(T, B, N);
    vec3 normal = mat.normalId > -1 ? texture(sampler2D(textures[mat.normalId], samp), inUV).rgb : inNormal;
    return TBN * normalize(normal);
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
    vec3 uvw = vec3(1.0f);
	vec4 outFragColor = texture(samplerCube(skybox, sampCubeMap), uvw);
    vec3 albedo = mat.baseColorId > -1 ? texture(sampler2D(textures[mat.baseColorId], samp), inUV).rgb : mat.baseColorFactor.xyz;
    float metallic = mat.metallicRoughnessId > -1 ? texture(sampler2D(textures[mat.metallicRoughnessId], samp), inUV).r : mat.metallicFactor;
    float roughness = mat.metallicRoughnessId > -1 ? texture(sampler2D(textures[mat.metallicRoughnessId], samp), inUV).g : mat.roughnessFactor;
    vec3 ao = vec3(1.0f);

    if (mat.occlusionId > -1){
        if (mat.occlusionId == mat.metallicRoughnessId){
            ao = texture(sampler2D(textures[mat.occlusionId], samp), inUV).aaa;
        }
        else{
            ao = texture(sampler2D(textures[mat.occlusionId], samp), inUV).rgb;
        }
    }
    vec3 emissive = mat.emissiveId > -1 ? texture(sampler2D(textures[mat.emissiveId], samp), inUV).rgb : mat.emissiveFactor.xyz;

    vec3 N = getNormal();
    vec3 V = normalize(cameras[0].viewPos - inWorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0f);

    for (int i = 0; i < lightCount; i++)
    {
        vec3 L = normalize(-lights[0].direction);
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
