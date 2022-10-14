#include "light.h"
#include "sceneManager.h"

namespace vkl {

// point light scene data
struct DirectionalLightLayout {
    glm::vec4 direction;
    glm::vec4 diffuse;
    glm::vec4 specular;
    DirectionalLightLayout(glm::vec4 direction, glm::vec4 diffuse, glm::vec4 specular)
        : direction(direction), diffuse(diffuse), specular(specular) {
    }
};

// point light scene data
struct PointLightLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;

    PointLightLayout(glm::vec4 position,
                     glm::vec4 diffuse,
                     glm::vec4 specular,
                     glm::vec4 attenuationFactor)
        : position(position), diffuse(diffuse), specular(specular), attenuationFactor(attenuationFactor) {
    }
};

Light::Light(IdType id)
    : UniformObject(id),
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
        data     = std::make_shared<DirectionalLightLayout>(
            _direction,
            _diffuse,
            _specular);
    } break;
    case LightType::POINT: {
        dataSize = sizeof(PointLightLayout);
        data     = std::make_shared<PointLightLayout>(
            _position,
            _diffuse,
            _specular,
            _attenuationFactor);

    } break;
    }
}
void Light::update() {
    switch (_type) {
    case LightType::DIRECTIONAL: {
        auto pData       = std::static_pointer_cast<DirectionalLightLayout>(data);
        pData->direction = _direction;
        pData->diffuse   = _diffuse;
        pData->specular  = _specular;
    } break;
    case LightType::POINT: {
        auto pData      = std::static_pointer_cast<PointLightLayout>(data);
        pData->position = _position;
        pData->diffuse  = _diffuse;
        pData->specular = _specular;
    } break;
    }
}
} // namespace vkl
