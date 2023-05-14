#include "descriptorSetLayout.h"
#include "device.h"
#include "descriptorPool.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const VkDescriptorSetLayoutCreateInfo& createInfo,
                                         VkDescriptorSetLayout handle) :
    m_pDevice(device)
{
    for(uint32_t idx = 0; idx < createInfo.bindingCount; idx++)
    {
        m_bindings.push_back(createInfo.pBindings[idx]);
    }
    m_pool      = std::make_unique<DescriptorPool>(this);
    getHandle() = handle;
}

VkDescriptorSet DescriptorSetLayout::allocateSet()
{
    return m_pool->allocateSet();
}

VkResult DescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return m_pool->freeSet(set);
}
}  // namespace aph::vk
