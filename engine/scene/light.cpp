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
Light::~Light(){
    switch (_type) {
    case LightType::POINT:
        delete static_cast<PointLightLayout*>(data);
        break;
    case LightType::DIRECTIONAL:
        delete static_cast<DirectionalLightLayout*>(data);
        break;
    }
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
void Light::load() {
    update();
    isloaded = true;
}
void Light::update() {
    if (isloaded){
        needUpdate = true;
    }
    switch (_type) {
    case LightType::DIRECTIONAL:
        {
            if (!isloaded){
                data = new DirectionalLightLayout;
            }
            DirectionalLightLayout d{
                .direction = _direction,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
            };
            dataSize = sizeof(DirectionalLightLayout);
            memcpy(data, &d, dataSize);
        }
        break;
    case LightType::POINT:
        {
            if (!isloaded){
                data = new PointLightLayout;
            }
            PointLightLayout d{
                .position = _position,
                .ambient = _manager->getAmbient(),
                .diffuse = _diffuse,
                .specular = _specular,
                .attenuationFactor = _attenuationFactor,
            };
            dataSize = sizeof(PointLightLayout);
            memcpy(data, &d, dataSize);
        }
        break;
    }
}
void *Light::getData() {
    return data;
}
uint32_t Light::getDataSize() {
    return dataSize;
}
} // namespace vkl
