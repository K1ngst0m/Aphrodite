#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outColor;
layout(location = 4) out vec4 outTangent;

layout (set = 0, binding = 1) uniform SceneUB{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} sceneData[];

//push constants block
layout( push_constant ) uniform constants
{
    mat4 modelMatrix;
};

void main() {
    gl_Position = sceneData[0].proj * sceneData[0].view * modelMatrix * vec4(inPosition, 1.0f);
    outWorldPos = vec3(modelMatrix * vec4(inPosition, 1.0f));
    outUV = inTexCoord;
    outNormal = mat3(modelMatrix) * inNormal;
    outColor = inColor;
    outTangent = inTangent;
}
