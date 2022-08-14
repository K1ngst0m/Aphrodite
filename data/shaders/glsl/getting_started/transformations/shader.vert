#version 450

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} cameraData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTexCoord;

layout(location = 0) out vec3 fragTexCoord;

//push constants block
layout( push_constant ) uniform constants
{
    vec4 data;
    mat4 modelMatrix;
} objectData;

void main() {
    gl_Position = cameraData.viewProj * objectData.modelMatrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
