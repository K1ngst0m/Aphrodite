#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace aph
{

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice*                        device,
                                                     const std::vector<ResourcesBinding>& bindings,
                                                     VkDescriptorSetLayout                handle) :
    m_device(device)
{
    m_bindings  = bindings;
    m_pool      = new VulkanDescriptorPool(this);
    getHandle() = handle;
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateSet() { return m_pool->allocateSet(); }

VkResult VulkanDescriptorSetLayout::freeSet(VkDescriptorSet set) { return m_pool->freeSet(set); }

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() { delete m_pool; }
}  // namespace aph
