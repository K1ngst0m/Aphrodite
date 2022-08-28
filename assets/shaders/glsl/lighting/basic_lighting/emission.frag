#version 450

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec3 position;
    vec3 color;
} pointLightData;

void main() {
    outColor = vec4(pointLightData.color, 1.0f);
}
