#ifndef VKLMATERIAL_H_
#define VKLMATERIAL_H_

#include "vklDevice.h"
#include "vklInit.hpp"
#include "vklTexture.h"
#include "vklUtils.h"

namespace vkl {
struct Material {
    glm::vec4 baseColorFactor       = glm::vec4(1.0f);
    vkl::Texture * baseColorTexture = nullptr;

    VkDescriptorSet descriptorSet;
};
} // namespace vkl

#endif // VKLMATERIAL_H_
