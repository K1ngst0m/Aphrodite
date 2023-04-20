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

struct Light : public Object
{
    Light();
    ~Light() override = default;

    glm::vec3 m_color{1.0f};
    glm::vec3 m_position{1.2f, 1.0f, 2.0f};
    glm::vec3 m_direction{-0.2f, -1.0f, -0.3f};
    LightType m_type{LightType::DIRECTIONAL};
};
}  // namespace aph

#endif  // VKLLIGHT_H_
