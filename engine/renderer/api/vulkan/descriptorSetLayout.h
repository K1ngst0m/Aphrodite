#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "device.h"

namespace vkl {
class VulkanDescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout> {
public:
    static VulkanDescriptorSetLayout *Create(VulkanDevice *device, VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                             VkDescriptorSetLayout handle);

    std::vector<VkDescriptorSetLayoutBinding> getBindings();
    VulkanDevice                             *getDevice();
    VkDescriptorSet                           allocateSet();
    VkResult                                  freeSet(VkDescriptorSet set);

private:
    VulkanDevice                             *_device;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
    VulkanDescriptorPool                     *_pool;
};

} // namespace vkl

#endif // DESCRIPTORSET_H_
