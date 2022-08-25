#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (set = 0, binding = 0) uniform SceneUB{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec3 viewPos;
} sceneData;

layout (set = 1, binding = 0) uniform PointLightUB{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuationFactor;
} pointLightData;

layout (set = 1, binding = 1) uniform DirectionalLightUB{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} directionalLightData;

layout(set = 2, binding = 0) uniform sampler2D sampler_baseColor;

void main() {
    outColor = texture(sampler_baseColor, fragTexCoord);
}
