#include "commandPool.h"
#include "queue.h"
#include "device.h"

namespace aph::vk
{
CommandPool::CommandPool(Device* pDevice, const CreateInfoType& createInfo, HandleType pool) :
    ResourceHandle(pool, createInfo),
    m_pDevice(pDevice),
    m_pQueue(createInfo.queue)
{
}
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
        APH_ASSERT(!m_allocatedCommandBuffers.count(ppCommandBuffers[i]));
        m_allocatedCommandBuffers.insert(ppCommandBuffers[i]);
    }
    CM_LOG_DEBUG("command buffer allocate, count %ld.", m_allocatedCommandBuffers.size());
    return Result::Success;
}

CommandBuffer* CommandPool::allocate()
{
    CommandBuffer* pCmd = {};
    APH_VR(allocate(1, &pCmd));
    return pCmd;
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
            m_pDevice->getDeviceTable()->vkFreeCommandBuffers(m_pDevice->getHandle(), getHandle(), 1, &ppCommandBuffers[i]->getHandle());
            m_allocatedCommandBuffers.erase(ppCommandBuffers[i]);
            m_commandBufferPool.free(ppCommandBuffers[i]);
        }
    }
}

void CommandPool::trim()
{
    std::lock_guard<std::mutex> holder{m_lock};
    m_pDevice->getDeviceTable()->vkTrimCommandPool(m_pDevice->getHandle(), getHandle(), 0);
}

void CommandPool::reset(bool freeMemory)
{
    std::lock_guard<std::mutex> holder{m_lock};
    m_pDevice->getDeviceTable()->vkResetCommandPool(m_pDevice->getHandle(), getHandle(),
                                                    freeMemory ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
    if (freeMemory)
    {
        for (auto cmd: m_allocatedCommandBuffers)
        {
            m_pDevice->getDeviceTable()->vkFreeCommandBuffers(m_pDevice->getHandle(), getHandle(), 1, &cmd->getHandle());
        }
        m_allocatedCommandBuffers.clear();
        m_commandBufferPool.clear();
    }
}

CommandPool::~CommandPool() = default;

Result CommandPoolAllocator::acquire(const CommandPoolCreateInfo& createInfo, uint32_t count,
                                     CommandPool** ppCommandPool)
{
    std::scoped_lock lock{m_lock};
    QueueType        queueType = createInfo.queue->getType();

    if(m_availablePools.contains(queueType))
    {
        auto& availPools = m_availablePools[queueType];

        while(!availPools.empty())
        {
            *ppCommandPool       = availPools.front();
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
        CM_LOG_DEBUG("command pool [%s] created", aph::vk::utils::toString(queueType));
        // utils::setDebugObjectName(m_pDevice->getHandle(), VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)pool, debugName);
        allPools.emplace(ppCommandPool[i]);
    }

    CM_LOG_DEBUG("command pool [%s] acquire, avail count %ld, all count %ld", aph::vk::utils::toString(queueType), m_availablePools[queueType].size(), allPools.size());
    return Result::Success;
}

void CommandPoolAllocator::release(uint32_t count, CommandPool** ppCommandPool)
{
    std::scoped_lock lock{m_lock};

    for(auto i = 0; i < count; ++i)
    {
        auto& pPool     = ppCommandPool[i];
        auto  queueType = pPool->getCreateInfo().queue->getType();

        pPool->reset(true);
        APH_ASSERT(m_allPools.contains(queueType) && m_allPools.at(queueType).contains(pPool));
        m_availablePools[queueType].push(pPool);
        CM_LOG_DEBUG("command pool [%s] released, avail count %ld, all count %ld",
                     vk::utils::toString(queueType),
                     m_availablePools[queueType].size(),
                     m_allPools[queueType].size());
    }
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
}  // namespace aph::vk
