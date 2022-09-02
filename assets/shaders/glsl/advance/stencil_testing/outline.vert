#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout (set = 0, binding = 0) uniform SceneUB{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 viewPos;
} sceneData;

//push constants block
layout( push_constant ) uniform constants
{
    mat4 modelMatrix;
} objectData;

void main() {
    vec3 pos = inPosition.xyz + inNormal * 0.025f;
    gl_Position = sceneData.viewProj * objectData.modelMatrix * vec4(pos, 1.0f);
}
