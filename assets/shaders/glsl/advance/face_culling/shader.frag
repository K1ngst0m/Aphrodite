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

layout(set = 1, binding = 0) uniform sampler2D sampler_baseColor;

void main() {
    vec4 color = texture(sampler_baseColor, fragTexCoord) * vec4(fragColor, 1.0);
    outColor = vec4(color);
}
