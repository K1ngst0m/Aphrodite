#include "light.h"

namespace vkl
{

struct LightData
{
    glm::vec4 color;
    glm::vec4 position;
    glm::vec4 direction;
    uint32_t type;  // 0 : point, 1 : directional, 2 : flash
};

void Light::load()
{
    data = std::make_shared<LightData>();
    dataSize = sizeof(LightData);
    auto pData = std::static_pointer_cast<LightData>(data);
    pData->direction = glm::vec4(_direction, 1.0f);
    pData->position = glm::vec4(_position, 1.0f);
    pData->type = static_cast<uint32_t>(_type);
}

void Light::update(float deltaTime)
{
    auto pData = std::static_pointer_cast<LightData>(data);
    pData->direction = glm::vec4(_direction, 1.0f);
    pData->position = glm::vec4(_position, 1.0f);
    pData->type = static_cast<uint32_t>(_type);
}

}  // namespace vkl
