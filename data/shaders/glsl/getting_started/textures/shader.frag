#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler_container;
layout(binding = 2) uniform sampler2D texSampler_awesomeface;

void main() {
    outColor = mix(texture(texSampler_container, fragTexCoord), texture(texSampler_awesomeface, fragTexCoord), 0.2);
}
