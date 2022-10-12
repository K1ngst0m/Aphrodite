#ifndef DESCRIPTORSETLAYOUT_H_
#define DESCRIPTORSETLAYOUT_H_

#include "device.h"

namespace vkl {
typedef std::vector<uint32_t> DescriptorSetLayoutHash;

class VulkanDescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout> {
public:
    static VkResult Create(VulkanDevice *device, const std::vector<PipelineResource> &setResources, VulkanDescriptorSetLayout **ppLayout);

    ~VulkanDescriptorSetLayout();

    const VulkanDevice *getDevice();

    std::vector<VkDescriptorSetLayoutBinding> getBindings();

    bool getLayoutBinding(uint32_t bindingIndex, VkDescriptorSetLayoutBinding **pBinding);

    VkDescriptorSet allocateDescriptorSet();

    VkResult freeDescriptorSet(VkDescriptorSet descriptorSet);

private:
    VulkanDevice                                              *_device = nullptr;
    std::vector<VkDescriptorSetLayoutBinding>                  _bindings;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _bindingsLookup;
    VulkanDescriptorPool                                      *_descriptorPool;
};
} // namespace vkl

#endif // DESCRIPTORSETLAYOUT_H_
