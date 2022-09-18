#ifndef VKLMATERIAL_H_
#define VKLMATERIAL_H_

#include "api/device.h"
#include "api/vkInit.hpp"
#include "api/texture.h"
#include "api/vkUtils.h"

namespace vkl {
struct Material {
    glm::vec4 baseColorFactor       = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex = 0;
};
} // namespace vkl

#endif // VKLMATERIAL_H_