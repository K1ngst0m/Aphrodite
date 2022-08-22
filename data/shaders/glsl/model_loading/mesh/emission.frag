#version 450

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} pointLightData;

void main() {
    vec4 result = vec4(pointLightData.ambient + pointLightData.diffuse + pointLightData.specular, 1.0f);
    outColor = result;
}
