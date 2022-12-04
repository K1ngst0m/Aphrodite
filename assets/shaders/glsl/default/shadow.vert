#version 450 core
layout (location = 0) in vec3 aPos;

layout (set = 0, binding = 0) uniform SceneUB{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} cameraData[2];

layout( push_constant ) uniform constant
{
    mat4 model;
};

void main()
{
    gl_Position = cameraData[1].proj * cameraData[1].view * model * vec4(aPos, 1.0);
}
