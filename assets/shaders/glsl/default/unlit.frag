#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D samplerBaseColor;

vec3 Uncharted2Tonemap(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color)
{
    float exposure = 1.0f;
    float gamma = 2.2f;
    vec3 outcol = Uncharted2Tonemap(color.rgb * exposure);
    outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    return vec4(pow(outcol, vec3(1.0f / gamma)), color.a);
}

void main() {
    vec4 color = texture(samplerBaseColor, fragTexCoord) * vec4(fragColor, 1.0);

    outColor = tonemap(color);
}
