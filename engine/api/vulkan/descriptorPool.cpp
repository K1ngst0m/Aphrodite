#include "descriptorPool.h"
#include "device.h"

namespace aph
{
VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorSetLayout* layout) : m_layout(layout)
{
    const auto& bindings = layout->getBindings();

    for(auto& binding : bindings)
    {
        m_descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }

    m_poolSizes.resize(m_descriptorTypeCounts.size());
    uint32_t index = 0;
    for(auto [type, count] : m_descriptorTypeCounts)
    {
        m_poolSizes[index].type            = type;
        m_poolSizes[index].descriptorCount = count * m_maxSetsPerPool;
        ++index;
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
    // Destroy all allocated descriptor sets.
    for(auto it : m_allocatedDescriptorSets)
    {
        vkFreeDescriptorSets(m_layout->getDevice()->getHandle(), m_pools[it.second], 1, &it.first);
    }

    // Destroy all created pools.
    for(auto pool : m_pools)
    {
        vkDestroyDescriptorPool(m_layout->getDevice()->getHandle(), pool, nullptr);
    }
}

VkDescriptorSet VulkanDescriptorPool::allocateSet()
{
    // Safe guard access to internal resources across threads.
    m_spinLock.Lock();

    // Find the next pool to allocate from.
    while(true)
    {
        // Allocate a new VkDescriptorPool if necessary.
        if(m_pools.size() <= m_currentAllocationPoolIndex)
        {
            // Create the Vulkan descriptor pool.
            VkDescriptorPoolCreateInfo createInfo = {
                .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets       = m_maxSetsPerPool,
                .poolSizeCount = static_cast<uint32_t>(m_poolSizes.size()),
                .pPoolSizes    = m_poolSizes.data(),
            };
            VkDescriptorPoolInlineUniformBlockCreateInfo descriptorPoolInlineUniformBlockCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO,
                .maxInlineUniformBlockBindings =
                    static_cast<uint32_t>(m_descriptorTypeCounts[VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK]),
            };
            if(m_descriptorTypeCounts.count(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK))
            {
                createInfo.pNext = &descriptorPoolInlineUniformBlockCreateInfo;
            }
            VkDescriptorPool handle = VK_NULL_HANDLE;
            auto result = vkCreateDescriptorPool(m_layout->getDevice()->getHandle(), &createInfo, nullptr, &handle);
            if(result != VK_SUCCESS)
                return VK_NULL_HANDLE;

            // Add the Vulkan handle to the descriptor pool instance.
            m_pools.push_back(handle);
            m_allocatedSets.push_back(0);
            break;
        }

        if(m_allocatedSets[m_currentAllocationPoolIndex] < m_maxSetsPerPool)
            break;

        // Increment pool index.
        ++m_currentAllocationPoolIndex;
    }

    // Increment allocated set count for given pool.
    ++m_allocatedSets[m_currentAllocationPoolIndex];

    // Allocate a new descriptor set from the current pool index.
    VkDescriptorSetLayout setLayout = m_layout->getHandle();

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = m_pools[m_currentAllocationPoolIndex];
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &setLayout;
    VkDescriptorSet handle                = VK_NULL_HANDLE;
    auto            result = vkAllocateDescriptorSets(m_layout->getDevice()->getHandle(), &allocInfo, &handle);
    if(result != VK_SUCCESS)
        return VK_NULL_HANDLE;

    // Store an internal mapping between the descriptor set handle and it's parent pool.
    // This is used when FreeDescriptorSet is called downstream.
    m_allocatedDescriptorSets.emplace(handle, m_currentAllocationPoolIndex);

    // Unlock access to internal resources.
    m_spinLock.Unlock();

    // Return descriptor set handle.
    return handle;
}

VkResult VulkanDescriptorPool::freeSet(VkDescriptorSet descriptorSet)
{
    // Safe guard access to internal resources across threads.
    m_spinLock.Lock();

    // Get the index of the descriptor pool the descriptor set was allocated from.
    auto it = m_allocatedDescriptorSets.find(descriptorSet);
    if(it == m_allocatedDescriptorSets.end())
        return VK_INCOMPLETE;

    // Return the descriptor set to the original pool.
    auto poolIndex = it->second;
    vkFreeDescriptorSets(m_layout->getDevice()->getHandle(), m_pools[poolIndex], 1, &descriptorSet);

    // Remove descriptor set from allocatedDescriptorSets map.
    m_allocatedDescriptorSets.erase(descriptorSet);

    // Decrement the number of allocated descriptor sets for the pool.
    --m_allocatedSets[poolIndex];

    // Set the next allocation to use this pool index.
    m_currentAllocationPoolIndex = poolIndex;

    // Unlock access to internal resources.
    m_spinLock.Unlock();

    // Return success.
    return VK_SUCCESS;
}
}  // namespace aph
