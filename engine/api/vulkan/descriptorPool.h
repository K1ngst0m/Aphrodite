#ifndef DESCRIPTORPOOL_H_
#define DESCRIPTORPOOL_H_

#include "common/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class DescriptorSetLayout;
class DescriptorPool : ResourceHandle<VkDescriptorPool>
{
public:
    DescriptorPool(DescriptorSetLayout* layout);
    ~DescriptorPool();

    VkDescriptorSet allocateSet();

    VkResult freeSet(VkDescriptorSet descriptorSet);

private:
    DescriptorSetLayout*              m_layout         = {};
    std::vector<VkDescriptorPoolSize> m_poolSizes      = {};
    uint32_t                          m_maxSetsPerPool = {50};

    std::vector<VkDescriptorPool>                  m_pools                      = {};
    std::vector<uint32_t>                          m_allocatedSets              = {};
    uint32_t                                       m_currentAllocationPoolIndex = {};
    std::unordered_map<VkDescriptorSet, uint32_t>  m_allocatedDescriptorSets    = {};
    std::unordered_map<VkDescriptorType, uint32_t> m_descriptorTypeCounts       = {};
    SpinLock                                       m_spinLock                   = {};
};
}  // namespace aph::vk

#endif  // DESCRIPTORPOOL_H_
