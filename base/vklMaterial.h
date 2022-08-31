#ifndef VKLMATERIAL_H_
#define VKLMATERIAL_H_

#include "vklUtils.h"
#include "vklTexture.h"
#include "vklInit.hpp"
#include "vklDevice.h"

namespace vkl {
struct Material {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex = 0;

    vkl::Texture *baseColorTexture = nullptr;
    vkl::Texture *specularTexture = nullptr;
};
}

#endif // VKLMATERIAL_H_
