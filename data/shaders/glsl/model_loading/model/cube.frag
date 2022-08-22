#version 450

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec4 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (set = 0, binding = 0) uniform SceneUB{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 viewPos;
} sceneData;

layout (set = 0, binding = 1) uniform PointLightUB{
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec3 attenuationFactor;
} pointLightData;

layout (set = 0, binding = 2) uniform DirectionalLightUB{
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} directionalLightData;

layout(set = 1, binding = 0) uniform sampler2D sampler_baseColor;

void main() {
    outColor = texture(sampler_baseColor, fragTexCoord);
}
