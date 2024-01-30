#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outColor;
layout(location = 4) out vec3 outTangent;

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

void main() {
    gl_Position = cameras[0].proj * cameras[0].view * modelMats[id] * vec4(inPosition, 1.0f);

	mat3 mNormal = transpose(inverse(mat3(modelMats[id])));

    outWorldPos = vec3(modelMats[id] * vec4(inPosition, 1.0f));
    outNormal = mNormal * inNormal;
    outUV = inTexCoord;
    outColor = inColor;
    outTangent = mNormal * inTangent;
}
