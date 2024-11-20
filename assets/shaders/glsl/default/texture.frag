#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    vec4 sampledColor = texture(tex, fragUV);
    outColor = sampledColor;
}
