#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;
layout(location = 4) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D samplerBaseColor;

void main() {
    vec4 color = texture(samplerBaseColor, fragTexCoord) * vec4(fragColor, 1.0);

    outColor = color;
}
