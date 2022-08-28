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

layout (set = 0, binding = 1) uniform PointLightUB{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuationFactor;
} pointLightData;

layout (set = 0, binding = 2) uniform DirectionalLightUB{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} directionalLightData;

layout(set = 1, binding = 0) uniform sampler2D sampler_baseColor;

void main() {
    vec4 color = texture(sampler_baseColor, fragTexCoord) * vec4(fragColor, 1.0);

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(directionalLightData.direction);
    vec3 V = normalize(sceneData.viewPos - fragPosition);
    vec3 R = reflect(L, N);
    vec3 diffuse = max(dot(N, L), 0.15) * fragColor;
    vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);

    outColor = vec4(diffuse * color.rgb + specular, 1.0f);
}
