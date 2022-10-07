#ifndef DESCRIPTORPOOL_H_
#define DESCRIPTORPOOL_H_

#include "common/spinlock.h"
#include "device.h"

namespace vkl {
class VulkanDescriptorPool {
public:
    VulkanDescriptorPool(VulkanDescriptorSetLayout *layout);

    ~VulkanDescriptorPool();

    VkDescriptorSet allocateDescriptorSet();

    VkResult freeDescriptorSet(VkDescriptorSet descriptorSet);

private:
    VulkanDescriptorSetLayout        *_layout = nullptr;
    std::vector<VkDescriptorPoolSize> _poolSizes;
    std::vector<VkDescriptorPool>     _pools;
    std::vector<uint32_t>             _allocatedSets;
    uint32_t                          _currentAllocationPoolIndex = 0;
    uint32_t                          _maxSetsPerPool             = 50;

    std::unordered_map<VkDescriptorSet, uint32_t> _allocatedDescriptorSets;
    SpinLock                                      _spinLock;
};
} // namespace vkl

#endif // DESCRIPTORPOOL_H_
