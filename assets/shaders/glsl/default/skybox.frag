#version 450

layout (set = 0, binding = 6) uniform textureCube skybox;
layout (set = 1, binding = 1) uniform sampler sampCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = texture(samplerCube(skybox, sampCubeMap), inUVW);
}
