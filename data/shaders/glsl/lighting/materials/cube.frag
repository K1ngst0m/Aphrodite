#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (set = 0, binding = 1) uniform SceneUB{
    vec3 viewPos;
    vec3 ambientColor;
} sceneData;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec3 position;
    vec3 color;
} pointLightData;

// set 1: per material binding
layout (set = 1, binding = 0) uniform MaterialUB{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float     shininess;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D texSampler_container;
layout(set = 1, binding = 2) uniform sampler2D texSampler_awesomeface;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(pointLightData.position - fragPosition);
    vec3 V = normalize(sceneData.viewPos - fragPosition);
    vec3 R = reflect(-L, N);

    vec3 ambient = sceneData.ambientColor * materialData.ambient;

    float diff = max(dot(N, L), 0.0f);
    vec3 diffuse = diff * materialData.diffuse * pointLightData.color;

    float spec = pow(max(dot(V, R), 0.0), materialData.shininess);
    vec3 specular = materialData.specular * spec * pointLightData.color;

    vec3 result = ambient + diffuse + specular;

    outColor = vec4(result, 1.0f);
}
