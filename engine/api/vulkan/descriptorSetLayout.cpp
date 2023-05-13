#include "descriptorSetLayout.h"
#include "device.h"
#include "descriptorPool.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const std::vector<ResourcesBinding>& bindings,
                                         VkDescriptorSetLayout handle) :
    m_pDevice(device)
{
    m_bindings  = bindings;
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
