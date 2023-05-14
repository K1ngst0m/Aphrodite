#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

// set 0: per scene binding
layout (std140, set = 0, binding = 0) uniform SceneInfoUB{
    vec4 ambientColor;
    uint cameraCount;
    uint lightCount;
};

struct Camera{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
};

layout (set = 0, binding = 2) uniform CameraUB{
    Camera cameras[10];
};

struct Light{
    vec4 color;
    vec4 position;
    vec4 direction;
    uint lightType;
};
layout (set = 0, binding = 3) uniform LightUB{
    Light lights[10];
};

layout (set = 2, binding = 0) uniform texture2D positionMap;
layout (set = 2, binding = 1) uniform texture2D normalMap;
layout (set = 2, binding = 2) uniform texture2D albedoMap;
layout (set = 2, binding = 3) uniform texture2D metallicRoughnessAOMap;
layout (set = 2, binding = 4) uniform texture2D emissiveMap;
layout (set = 2, binding = 5) uniform texture2D shadowMap;

layout (set = 1, binding = 0) uniform sampler samp;
layout (set = 1, binding = 1) uniform sampler smp1;
layout (set = 1, binding = 2) uniform sampler shadowSamp;

const float PI = 3.14159265359;

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

float textureProj(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture(sampler2D(shadowMap, shadowSamp), shadowCoord.st + off).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z)
        {
            shadow = ambientColor.x;
        }
    }
    return shadow;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = cameras[1].viewPos.xyz / cameras[1].viewPos.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(sampler2D(shadowMap, shadowSamp), projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}


void main() {
    float shadowDist = texture(sampler2D(shadowMap, shadowSamp), inUV).r;

    // Get G-Buffer values
    vec3 fragPos = texture(sampler2D(positionMap, samp), inUV).rgb;
    vec3 albedo = texture(sampler2D(albedoMap, samp), inUV).rgb;
    vec3 emissive = texture(sampler2D(emissiveMap, samp), inUV).rgb;
    vec3 metallicRoughnessAO = texture(sampler2D(metallicRoughnessAOMap, samp), inUV).rgb;
    float metallic = metallicRoughnessAO.x;
    float roughness = metallicRoughnessAO.y;
    vec3 ao = vec3(metallicRoughnessAO.z);

    vec3 N = texture(sampler2D(normalMap, samp), inUV).rgb;
    vec3 V = normalize(cameras[0].viewPos.xyz - fragPos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 Lo = vec3(0.0f);

    for (int i = 0; i < lightCount; i++)
    {
        vec3 L = vec3(1.0f);
        if (lights[i].lightType == 0)
        {
            L = normalize(lights[i].position.xyz - fragPos);
        }
        else if (lights[i].lightType == 1)
        {
            L = normalize(-lights[i].direction.xyz);
        }
        else {
            L = vec3(-1.0f);
        }
        vec3 H = normalize(V + L);
        vec3 R = reflect(L, N);
        vec3 radiance = lights[i].color.xyz;

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

        float shadow = ShadowCalculation(cameras[1].view * cameras[1].viewPos);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        Lo *= shadow;
    }

    vec3 ambient = ambientColor.xyz * albedo * ao;
    vec3 color = emissive + Lo + ambient;

    outFragcolor = vec4(color, 1.0f);
}
