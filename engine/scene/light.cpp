#include "light.h"
#include "sceneManager.h"

namespace vkl {

// point light scene data
struct DirectionalLightLayout {
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    DirectionalLightLayout(glm::vec4 direction, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular)
        : direction(direction), ambient(ambient), diffuse(diffuse), specular(specular) {
    }
};

// point light scene data
struct PointLightLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;

    PointLightLayout(glm::vec4 position, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular)
        : position(position), ambient(ambient), diffuse(diffuse), specular(specular) {
    }
};

Light::Light(SceneManager *manager)
    : UniformObject(manager),
      _diffuse(0.5f, 0.5f, 0.5f, 1.0f),
      _specular(1.0f, 1.0f, 1.0f, 1.0f),
      _position(1.2f, 1.0f, 2.0f, 1.0f),
      _attenuationFactor(1.0f, 0.09f, 0.032f, 0.0f),
      _direction(-0.2f, -1.0f, -0.3f, 1.0f) {
}
Light::~Light() = default;
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
void Light::load() {
    switch (_type) {
    case LightType::DIRECTIONAL: {
        dataSize = sizeof(DirectionalLightLayout);
        if (!isloaded) {
            data = std::make_shared<DirectionalLightLayout>(
                _direction,
                _manager->getAmbient(),
                _diffuse,
                _specular);
        }
    } break;
    case LightType::POINT: {
        dataSize = sizeof(PointLightLayout);
        if (!isloaded) {
            data = std::make_shared<PointLightLayout>(
                _position,
                _diffuse,
                _specular,
                _attenuationFactor);
        }

    } break;
    }
    isloaded = true;
}
void Light::update() {
    switch (_type) {
    case LightType::DIRECTIONAL: {
        dataSize = sizeof(DirectionalLightLayout);
        auto pData = std::static_pointer_cast<DirectionalLightLayout>(data);
        pData->ambient = _manager->getAmbient();
        pData->direction = _direction;
        pData->diffuse = _diffuse;
        pData->specular = _specular;
    } break;
    case LightType::POINT: {
        dataSize = sizeof(PointLightLayout);
        auto pData = std::static_pointer_cast<PointLightLayout>(data);
        pData->ambient = _manager->getAmbient();
        pData->position = _position;
        pData->diffuse = _diffuse;
        pData->specular = _specular;
    } break;
    }
}
uint32_t Light::getDataSize() {
    return dataSize;
}
} // namespace vkl
