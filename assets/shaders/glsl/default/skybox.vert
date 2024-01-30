#version 450

layout (location = 0) in vec3 inPos;

struct Camera{
    mat4 view;
    mat4 proj;
    vec3 viewPos;
};
layout (set = 0, binding = 2) uniform CameraUB{
    Camera cameras[100];
};

layout (location = 0) out vec3 outUVW;

out gl_PerVertex
{
    vec4 gl_Position;
};


void main()
{
    outUVW = inPos;
    vec4 pos = cameras[0].proj * mat4(mat3(cameras[0].view)) * vec4(inPos.xyz, 1.0);
    gl_Position = pos.xyww;
}
