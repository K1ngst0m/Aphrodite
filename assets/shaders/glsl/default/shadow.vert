#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec3 inTangent;

layout (set = 0, binding = 1) uniform modelMatUB{
    mat4 modelMats[100];
};

struct Camera{
    mat4 view;
    mat4 proj;
    vec3 viewPos;
};
layout (set = 0, binding = 2) uniform CameraUB{
    Camera cameras[100];
};

layout( push_constant ) uniform constants
{
    uint id;
};

void main()
{
    gl_Position = cameras[1].proj * cameras[1].view * modelMats[id] * vec4(inPosition, 1.0f);
}
