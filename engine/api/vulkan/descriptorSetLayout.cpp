#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace aph
{

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice*                          device,
                                                     const VkDescriptorSetLayoutCreateInfo& createInfo,
                                                     VkDescriptorSetLayout                  handle) :
    m_device(device)
{
    for(uint32_t i = 0; i < createInfo.bindingCount; i++)
    {
        m_bindings.push_back(createInfo.pBindings[i]);
    }
    getHandle()     = handle;
    getCreateInfo() = createInfo;
    m_pool          = new VulkanDescriptorPool(this);
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateSet()
{
    return m_pool->allocateSet();
}

VkResult VulkanDescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return m_pool->freeSet(set);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    delete m_pool;
}
}  // namespace aph
