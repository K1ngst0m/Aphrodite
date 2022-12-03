#version 450 core
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D screenTexture;

void main()
{
    vec3 col = texture(screenTexture, inUV).rgb;
    outColor = vec4(col, 1.0);
}
