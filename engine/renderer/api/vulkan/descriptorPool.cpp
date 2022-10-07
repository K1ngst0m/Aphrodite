#include "descriptorPool.h"
#include "descriptorSetLayout.h"

namespace vkl {

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorSetLayout *layout)
    : _layout(layout) {
    // Get the layout's binding information.
    const auto &bindings = layout->getBindings();

    // Generate array of pool sizes for each unique resource type.
    std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts;
    for (auto &binding : bindings) {
        descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }

    // Fill in pool sizes array.
    _poolSizes.resize(descriptorTypeCounts.size());
    uint32_t index = 0;
    for (auto &it : descriptorTypeCounts) {
        _poolSizes[index].type            = it.first;
        _poolSizes[index].descriptorCount = it.second * _maxSetsPerPool;
        ++index;
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
    // Destroy all allocated descriptor sets.
    for (auto it : _allocatedDescriptorSets) {
        vkFreeDescriptorSets(_layout->getDevice()->getLogicalDevice(), _pools[it.second], 1, &it.first);
    }

    // Destroy all created pools.
    for (auto pool : _pools) {
        vkDestroyDescriptorPool(_layout->getDevice()->getLogicalDevice(), pool, nullptr);
    }
}

VkDescriptorSet VulkanDescriptorPool::allocateDescriptorSet() {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();

    // Find the next pool to allocate from.
    while (true) {
        // Allocate a new VkDescriptorPool if necessary.
        if (_pools.size() <= _currentAllocationPoolIndex) {
            // Create the Vulkan descriptor pool.
            VkDescriptorPoolCreateInfo createInfo = {};
            createInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            createInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            createInfo.poolSizeCount              = static_cast<uint32_t>(_poolSizes.size());
            createInfo.pPoolSizes                 = _poolSizes.data();
            createInfo.maxSets                    = _maxSetsPerPool;
            VkDescriptorPool handle               = VK_NULL_HANDLE;
            auto             result               = vkCreateDescriptorPool(_layout->getDevice()->getLogicalDevice(), &createInfo, nullptr, &handle);
            if (result != VK_SUCCESS) {
                return VK_NULL_HANDLE;
            }

            // Add the Vulkan handle to the descriptor pool instance.
            _pools.push_back(handle);
            _allocatedSets.push_back(0);
            break;
        }
        if (_allocatedSets[_currentAllocationPoolIndex] < _maxSetsPerPool) {
            break;
        }

        // Increment pool index.
        ++_currentAllocationPoolIndex;
    }

    // Increment allocated set count for given pool.
    ++_allocatedSets[_currentAllocationPoolIndex];

    // Allocate a new descriptor set from the current pool index.
    VkDescriptorSetLayout setLayout = _layout->getHandle();

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = _pools[_currentAllocationPoolIndex];
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &setLayout;
    VkDescriptorSet handle                = VK_NULL_HANDLE;
    auto            result                = vkAllocateDescriptorSets(_layout->getDevice()->getLogicalDevice(), &allocInfo, &handle);
    if (result != VK_SUCCESS)
        return VK_NULL_HANDLE;

    // Store an internal mapping between the descriptor set handle and it's parent pool.
    // This is used when FreeDescriptorSet is called downstream.
    _allocatedDescriptorSets.emplace(handle, _currentAllocationPoolIndex);

    // Unlock access to internal resources.
    _spinLock.Unlock();

    // Return descriptor set handle.
    return handle;
}

VkResult VulkanDescriptorPool::freeDescriptorSet(VkDescriptorSet descriptorSet) {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();

    // Get the index of the descriptor pool the descriptor set was allocated from.
    auto it = _allocatedDescriptorSets.find(descriptorSet);
    if (it == _allocatedDescriptorSets.end())
        return VK_INCOMPLETE;

    // Return the descriptor set to the original pool.
    auto poolIndex = it->second;
    vkFreeDescriptorSets(_layout->getDevice()->getLogicalDevice(), _pools[poolIndex], 1, &descriptorSet);

    // Remove descriptor set from allocatedDescriptorSets map.
    _allocatedDescriptorSets.erase(descriptorSet);

    // Decrement the number of allocated descriptor sets for the pool.
    --_allocatedSets[poolIndex];

    // Set the next allocation to use this pool index.
    _currentAllocationPoolIndex = poolIndex;

    // Unlock access to internal resources.
    _spinLock.Unlock();

    // Return success.
    return VK_SUCCESS;
}
} // namespace vkl
