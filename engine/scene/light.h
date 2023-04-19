#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "object.h"

namespace aph
{

enum class LightType : uint32_t
{
    POINT       = 0,
    DIRECTIONAL = 1,
    SPOT        = 2,
};

class Light : public Object
{
public:
    Light() : Object(Id::generateNewId<Light>(), ObjectType::LIGHT) {}
    ~Light() override = default;

    void setPosition(glm::vec3 value) { m_position = value; }
    void setDirection(glm::vec3 value) { m_direction = value; }
    void setColor(glm::vec3 value) { m_color = value; }
    void setType(LightType type) { m_type = type; }

    glm::vec3 getColor() const { return m_color; }
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getDirection() const { return m_direction; }
    LightType getType() const { return m_type; }

private:
    glm::vec3 m_color{1.0f};
    glm::vec3 m_position{1.2f, 1.0f, 2.0f};
    glm::vec3 m_direction{-0.2f, -1.0f, -0.3f};
    LightType m_type{LightType::DIRECTIONAL};
};
}  // namespace aph

#endif  // VKLLIGHT_H_
