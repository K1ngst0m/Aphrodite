#include "light.h"

namespace vkl {

struct LightData {
    glm::vec4 color;
    glm::vec4 position;
    glm::vec4 direction;
};

Light::Light(IdType id)
    : UniformObject(id),
      _color(1.0f),
      _position(1.2f, 1.0f, 2.0f),
      _direction(-0.2f, -1.0f, -0.3f),
      _type(LightType::DIRECTIONAL) {
}

Light::~Light() = default;

void Light::setPosition(glm::vec3 value) {
    _position = value;
}

void Light::setDirection(glm::vec3 value) {
    _direction = value;
}
void Light::setType(LightType type) {
    _type = type;
}
void Light::load() {
    data             = std::make_shared<LightData>();
    dataSize         = sizeof(LightData);
    auto pData       = std::static_pointer_cast<LightData>(data);
    pData->direction = glm::vec4(_direction, 1.0f);
    pData->position  = glm::vec4(_position, 1.0f);
}
void Light::update(float deltaTime) {
    auto pData       = std::static_pointer_cast<LightData>(data);
    pData->direction = glm::vec4(_direction, 1.0f);
    pData->position  = glm::vec4(_position, 1.0f);
}
std::shared_ptr<Light> Light::Create() {
    auto instance = std::make_shared<Light>(Id::generateNewId<Light>());
    return instance;
}
void Light::setColor(glm::vec3 value) {
    _color = value;
}
} // namespace vkl
