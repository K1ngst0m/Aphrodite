#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace vkl {

VulkanDescriptorSetLayout *VulkanDescriptorSetLayout::Create(VulkanDevice                    *device,
                                                             VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                             VkDescriptorSetLayout            handle) {
    auto instance = new VulkanDescriptorSetLayout;
    for (auto i = 0; i < pCreateInfo->bindingCount; i++) {
        instance->_bindings.push_back(pCreateInfo->pBindings[i]);
    }
    instance->_device = device;
    instance->_handle = handle;
    instance->_pool   = new VulkanDescriptorPool(instance);
    return instance;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptorSetLayout::getBindings() {
    return _bindings;
}

VulkanDevice *VulkanDescriptorSetLayout::getDevice() {
    return _device;
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateSet() {
    return _pool->allocateSet();
}

VkResult VulkanDescriptorSetLayout::freeSet(VkDescriptorSet set) {
    return _pool->freeSet(set);
}

} // namespace vkl
