#version 450

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} pointLightData;

void main() {
    vec4 result = pointLightData.ambient + pointLightData.diffuse + pointLightData.specular;
    outColor = result;
}
