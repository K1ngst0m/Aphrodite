#version 450

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec4 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (set = 0, binding = 1) uniform SceneUB{
    vec4 viewPos;
} sceneData;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} pointLightData;

layout (set = 0, binding = 3) uniform DirectionalLightUB{
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} directionalLightData;

// set 1: per material binding
layout (set = 1, binding = 0) uniform MaterialUB{
    float     shininess;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D texSampler_container_diffuse;
layout(set = 1, binding = 2) uniform sampler2D texSampler_container_specular;

void main() {
    vec3 N = normalize(fragNormal.xyz);
    vec3 V = normalize(sceneData.viewPos.xyz - fragPosition.xyz);

    vec4 ambient = vec4(0.0f);
    vec4 specular = vec4(0.0f);
    vec4 diffuse = vec4(0.0f);

    // direction light
    {
        vec3 L = normalize(-directionalLightData.direction.xyz);
        vec3 R = reflect(-L, N);

        ambient += directionalLightData.ambient * texture(texSampler_container_diffuse, fragTexCoord);

        float diff = max(dot(N, L), 0.0f);
        diffuse += diff * texture(texSampler_container_diffuse, fragTexCoord) * directionalLightData.diffuse;

        float spec = pow(max(dot(V, R), 0.0f), materialData.shininess);
        specular += texture(texSampler_container_specular, fragTexCoord) * spec * directionalLightData.specular;
    }

    // point light
    {

    }

    // spot light
    {

    }

    vec4 result = ambient + specular + diffuse;

    outColor = result;
}
