#version 450

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 6) uniform textureCube skybox;
layout (set = 1, binding = 1) uniform sampler sampCubeMap;
layout (set = 1, binding = 2) uniform sampler smp2;
layout (set = 1, binding = 0) uniform sampler smp0;

void main()
{
	outFragColor = texture(samplerCube(skybox, sampCubeMap), inUVW);
}
