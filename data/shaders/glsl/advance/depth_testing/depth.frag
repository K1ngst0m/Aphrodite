#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vec3(gl_FragCoord.z), 1.0f);
}
