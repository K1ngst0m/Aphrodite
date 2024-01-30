#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "math/math.h"

namespace aph
{
struct UIComponentDesc
{
    glm::vec2 offset = {0.0f, 150.0f};
    glm::vec2 size   = {600.0f, 550.0f};

    uint32_t fontID   = 0;
    float    fontSize = 16.0f;
};
}  // namespace aph

#endif  // UIRENDERER_H_
