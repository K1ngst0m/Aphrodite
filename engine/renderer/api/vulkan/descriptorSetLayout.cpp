#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace vkl
{

VulkanDescriptorSetLayout *VulkanDescriptorSetLayout::Create(
    VulkanDevice *device, VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayout handle)
{
    auto instance = new VulkanDescriptorSetLayout();
    for(auto i = 0; i < pCreateInfo->bindingCount; i++)
    {
        instance->_bindings.push_back(pCreateInfo->pBindings[i]);
    }
    instance->_device = device;
    instance->_handle = handle;
    instance->_pool = new VulkanDescriptorPool(instance);
    return instance;
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateSet()
{
    return _pool->allocateSet();
}
VkResult VulkanDescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return _pool->freeSet(set);
}
VulkanDescriptorSetLayout::VulkanDescriptorSetLayout()
{
    // _pool = new VulkanDescriptorPool(this);
}
VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    delete _pool;
}
}  // namespace vkl
