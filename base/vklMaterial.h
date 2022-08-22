#ifndef VKLMATERIAL_H_
#define VKLMATERIAL_H_

#include "vklUtils.h"
#include "vklTexture.h"

namespace vkl {
struct Material {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex = 0;
    glm::vec4 specularFactor = glm::vec4(1.0f);
    float shininess = 64.0f;

    vkl::Texture *baseColorTexture = nullptr;
    vkl::Texture *specularTexture = nullptr;

    VkDescriptorSet descriptorSet;

    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
};

}

#endif // VKLMATERIAL_H_
