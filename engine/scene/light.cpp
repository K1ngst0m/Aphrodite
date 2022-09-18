#include "light.h"
#include "sceneManager.h"

namespace vkl {

// point light scene data
struct DirectionalLightLayout {
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// point light scene data
struct PointLightLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;
};

Light::Light(SceneManager *manager)
    : UniformBufferObject(manager),
      _diffuse(0.5f, 0.5f, 0.5f, 1.0f),
      _specular(1.0f, 1.0f, 1.0f, 1.0f),
      _position(1.2f, 1.0f, 2.0f, 1.0f),
      _attenuationFactor(1.0f, 0.09f, 0.032f, 0.0f),
      _direction(-0.2f, -1.0f, -0.3f, 1.0f) {
}

void Light::setDiffuse(glm::vec4 value) {
    _diffuse = value;
}
void Light::setSpecular(glm::vec4 value) {
    _specular = value;
}
void Light::setPosition(glm::vec4 value) {
    _position = value;
}
void Light::setDirection(glm::vec4 value) {
    _direction = value;
}
void Light::setType(LightType type) {
    _type = type;
}
void Light::load(vkl::Device *device) {
    _manager->getAmbient();
    switch (_type) {
    case LightType::DIRECTIONAL:
        {
            DirectionalLightLayout data{
                .direction = _direction,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
            };
            UniformBufferObject::setupBuffer(device, sizeof(DirectionalLightLayout), &data);
        }
        break;
    case LightType::POINT:
        {
            PointLightLayout data{
                .position = _position,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
                .attenuationFactor = _attenuationFactor,
            };
            UniformBufferObject::setupBuffer(device, sizeof(PointLightLayout), &data);
        }
        break;
    }
}
void Light::update() {
    switch (_type) {
    case LightType::DIRECTIONAL:
        {
            DirectionalLightLayout data{
                .direction = _direction,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
            };
            UniformBufferObject::updateBuffer(&data);
        }
        break;
    case LightType::POINT:
        {
            PointLightLayout data{
                .position = _position,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
                .attenuationFactor = _attenuationFactor,
            };
            UniformBufferObject::updateBuffer(&data);
        }
        break;
    }
}
} // namespace vkl
