#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "object.h"

namespace vkl
{

enum class LightType : uint32_t
{
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2,
};

class Light : public UniformObject
{
public:
    Light() : UniformObject(Id::generateNewId<Light>(), ObjectType::LIGHT) {}
    ~Light() override = default;

    void setPosition(glm::vec3 value) { m_position = value; }
    void setDirection(glm::vec3 value) { m_direction = value; }

    void setColor(glm::vec3 value) { m_color = value; }
    void setType(LightType type) { m_type = type; }

    void load() override;
    void update(float deltaTime) override;

private:
    glm::vec3 m_color{ 1.0f };
    glm::vec3 m_position{ 1.2f, 1.0f, 2.0f };
    glm::vec3 m_direction{ -0.2f, -1.0f, -0.3f };
    LightType m_type{ LightType::DIRECTIONAL };
};
}  // namespace vkl

#endif  // VKLLIGHT_H_
