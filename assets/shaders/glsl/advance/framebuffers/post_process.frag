#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sampler_baseColor;

void main() {
    vec4 color = texture(sampler_baseColor, fragTexCoord);
    outColor = vec4(1.0f) - color;
}
