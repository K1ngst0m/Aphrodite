#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// set 0: per scene binding
layout (set = 0, binding = 1) uniform SceneUB{
    vec3 viewPos;
} sceneData;

layout (set = 0, binding = 2) uniform PointLightUB{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuationFactor;
} pointLightData;

layout (set = 0, binding = 3) uniform DirectionalLightUB{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} directionalLightData;

layout (set = 0, binding = 4) uniform FlashLightUB{
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;
} flashLightData;

layout(set = 1, binding = 0) uniform sampler2D texSampler_container_diffuse;
layout(set = 1, binding = 1) uniform sampler2D texSampler_container_specular;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(sceneData.viewPos - fragPosition);

    vec3 ambient = vec3(0.0f);
    vec3 specular = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);

    // direction light
    {
        vec3 L = normalize(-directionalLightData.direction);
        vec3 R = reflect(-L, N);

        ambient += directionalLightData.ambient * texture(texSampler_container_diffuse, fragTexCoord).rgb;

        float diff = max(dot(N, L), 0.0f);
        diffuse += diff * texture(texSampler_container_diffuse, fragTexCoord).rgb * directionalLightData.diffuse;

        float spec = pow(max(dot(V, R), 0.0f), 128.0f);
        specular += texture(texSampler_container_specular, fragTexCoord).rgb * spec * directionalLightData.specular;
    }

    // point light
    {
        float distance    = length(pointLightData.position - fragPosition);
        float attenuation = 1.0 / (pointLightData.attenuationFactor[0] + pointLightData.attenuationFactor[1] * distance + pointLightData.attenuationFactor[2] * (distance * distance));

        vec3 L = normalize(pointLightData.position - fragPosition);
        vec3 R = reflect(-L, N);

        ambient += pointLightData.ambient * texture(texSampler_container_diffuse, fragTexCoord).rgb;

        float diff = max(dot(N, L), 0.0f);
        diffuse += diff * texture(texSampler_container_diffuse, fragTexCoord).rgb * pointLightData.diffuse * attenuation;

        float spec = pow(max(dot(V, R), 0.0f), 128.0f);
        specular += texture(texSampler_container_specular, fragTexCoord).rgb * spec * pointLightData.specular * attenuation;
    }

    // flash light
    {
        vec3 L = normalize(flashLightData.position - fragPosition);
        vec3 R = reflect(-L, N);

        float theta = dot(L, normalize(-flashLightData.direction));
        float epsilon = flashLightData.cutOff - flashLightData.outerCutOff;
        float intensity = clamp((theta - flashLightData.outerCutOff) / epsilon, 0.0f, 1.0f);

        ambient += flashLightData.ambient * texture(texSampler_container_diffuse, fragTexCoord).rgb;

        float diff = max(dot(N, L), 0.0f);
        diffuse += diff * texture(texSampler_container_diffuse, fragTexCoord).rgb * flashLightData.diffuse * intensity;

        float spec = pow(max(dot(V, R), 0.0f), 128.0f);
        specular += texture(texSampler_container_specular, fragTexCoord).rgb * spec * flashLightData.specular * intensity;
    }

    vec4 result = vec4(ambient + specular + diffuse, 1.0f);

    outColor = result;
}
