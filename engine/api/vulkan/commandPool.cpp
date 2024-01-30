#include "commandPool.h"
#include "queue.h"
#include "device.h"

namespace aph::vk
{
Result CommandPool::allocate(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    // Allocate a new command buffer.
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = getHandle(),
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    std::vector<VkCommandBuffer> handles(count);
    std::lock_guard<std::mutex>  holder{m_lock};
    _VR(m_pDevice->getDeviceTable()->vkAllocateCommandBuffers(m_pDevice->getHandle(), &allocInfo, handles.data()));

    for(auto i = 0; i < count; i++)
    {
        ppCommandBuffers[i] = m_commandBufferPool.allocate(m_pDevice, handles[i], m_pQueue);
        m_allocatedCommandBuffers.push_back(ppCommandBuffers[i]);
    }
    CM_LOG_DEBUG("command buffer allocate, avail count %ld, all count %ld", m_allocatedCommandBuffers.size());
    return Result::Success;
}
void CommandPool::free(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    APH_ASSERT(ppCommandBuffers);

    std::lock_guard<std::mutex> holder{m_lock};
    // Destroy all of the command buffers.
    for(auto i = 0U; i < count; ++i)
    {
        if(ppCommandBuffers[i])
        {
            m_commandBufferPool.free(ppCommandBuffers[i]);
        }
    }
}

void CommandPool::reset(bool freeMemory)
{
    std::lock_guard<std::mutex> holder{m_lock};
    m_pDevice->getDeviceTable()->vkResetCommandPool(m_pDevice->getHandle(), getHandle(),
                                                    freeMemory ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
}

CommandPool::~CommandPool() = default;

Result CommandPoolAllocator::acquire(const CommandPoolCreateInfo& createInfo, uint32_t count,
                                     CommandPool** ppCommandPool)
{
    std::scoped_lock lock{m_lock};
    auto             queueType = createInfo.queue->getType();

    if(m_availablePools.contains(queueType))
    {
        auto& availPools = m_availablePools[queueType];

        while(!availPools.empty())
        {
            auto& cbPool = *ppCommandPool;
            cbPool       = availPools.front();
            availPools.pop();
            ++ppCommandPool;
            if(--count == 0)
            {
                break;
            }
        }
    }

    auto& allPools = m_allPools[queueType];
    for(auto i = 0; i < count; ++i)
    {
        VkCommandPoolCreateInfo cmdPoolInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = createInfo.queue->getFamilyIndex(),
        };

        if(createInfo.transient)
        {
            cmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }

        VkCommandPool pool;
        _VR(m_pDevice->getDeviceTable()->vkCreateCommandPool(m_pDevice->getHandle(), &cmdPoolInfo, vkAllocator(),
                                                             &pool));
        ppCommandPool[i] = m_resourcePool.allocate(m_pDevice, createInfo, pool);
        // utils::setDebugObjectName(m_pDevice->getHandle(), VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)pool, debugName);
        allPools.emplace(ppCommandPool[i]);
    }

    CM_LOG_DEBUG("command pool acquire, avail count %ld, all count %ld", m_availablePools.size(), m_allPools.size());
    return Result::Success;
}

void CommandPoolAllocator::release(uint32_t count, CommandPool** ppCommandPool)
{
    std::scoped_lock lock{m_lock};

    for(auto i = 0; i < count; ++i)
    {
        auto& pPool     = ppCommandPool[i];
        auto  queueType = pPool->getCreateInfo().queue->getType();

        if(m_allPools.contains(queueType) && m_allPools.at(queueType).contains(pPool))
        {
            m_availablePools[queueType].push(ppCommandPool[i]);
        }
    }
    CM_LOG_DEBUG("command pool release, avail count %ld, all count %ld", m_availablePools.size(), m_allPools.size());
}
void CommandPoolAllocator::clear()
{
    for(auto& [_, poolSet] : m_allPools)
    {
        for(auto& pool : poolSet)
        {
            m_pDevice->getDeviceTable()->vkDestroyCommandPool(m_pDevice->getHandle(), pool->getHandle(), vkAllocator());
        }
    }
    m_allPools.clear();
    m_availablePools.clear();
}

CommandBuffer* CommandPool::allocate()
{
    CommandBuffer* pCmd = {};
    APH_VR(allocate(1, &pCmd));
    return pCmd;
}
}  // namespace aph::vk
