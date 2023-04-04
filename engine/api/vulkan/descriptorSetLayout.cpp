#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace aph
{

VulkanDescriptorSetLayout *VulkanDescriptorSetLayout::Create(
    VulkanDevice *device, VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayout handle)
{
    auto instance = new VulkanDescriptorSetLayout();
    for(auto i = 0; i < pCreateInfo->bindingCount; i++)
    {
        instance->m_bindings.push_back(pCreateInfo->pBindings[i]);
    }
    instance->m_device = device;
    instance->getHandle() = handle;
    instance->m_pool = new VulkanDescriptorPool(instance);
    return instance;
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateSet()
{
    return m_pool->allocateSet();
}
VkResult VulkanDescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return m_pool->freeSet(set);
}
VulkanDescriptorSetLayout::VulkanDescriptorSetLayout()
{
    // _pool = new VulkanDescriptorPool(this);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    delete m_pool;
}
}  // namespace aph
