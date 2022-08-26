#include "vklMaterial.h"

namespace vkl {
void Material::createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
    assert(baseColorTexture);
    assert(specularTexture);

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

    std::vector<VkDescriptorImageInfo> imageDescriptors{};
    std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

    if (baseColorTexture){
        imageDescriptors.push_back(baseColorTexture->descriptorInfo);
        VkWriteDescriptorSet writeDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &baseColorTexture->descriptorInfo,
        };
        writeDescriptorSets.push_back(writeDescriptorSet);
    }

    if (specularTexture){
        imageDescriptors.push_back(specularTexture->descriptorInfo);
        VkWriteDescriptorSet writeDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &specularTexture->descriptorInfo,
        };
        writeDescriptorSets.push_back(writeDescriptorSet);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}
}
