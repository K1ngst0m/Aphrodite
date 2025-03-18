#pragma once

#include "object.h"

namespace aph
{

enum class LightType : uint32_t
{
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2,
};

struct Light : public Object
{
    Light();
    ~Light() override = default;

    float m_intensity{ 1.0f };
    glm::vec3 m_color{ 1.0f };
    LightType m_type{ LightType::DIRECTIONAL };
    union
    {
        glm::vec3 m_position;
        glm::vec3 m_direction;
    };
};
} // namespace aph

