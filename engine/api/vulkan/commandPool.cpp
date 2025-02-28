#include "commandPool.h"
#include "device.h"
#include "queue.h"

namespace aph::vk
{
CommandPool::CommandPool(Device* pDevice, const CreateInfoType& createInfo, HandleType pool)
    : ResourceHandle(pool, createInfo)
    , m_pDevice(pDevice)
    , m_pQueue(createInfo.queue)
{
}
CommandPool::~CommandPool() = default;

Result CommandPool::allocate(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    std::lock_guard<std::mutex> holder{ m_lock };

    // Allocate a new command buffer.
    ::vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.setCommandPool(getHandle()).setLevel(::vk::CommandBufferLevel::ePrimary).setCommandBufferCount(count);
    auto [result, handles] = m_pDevice->getHandle().allocateCommandBuffers(allocInfo);

    for (auto i = 0; i < count; i++)
    {
        ppCommandBuffers[i] = m_commandBufferPool.allocate(m_pDevice, handles[i], m_pQueue);
        APH_ASSERT(!m_allocatedCommandBuffers.contains(ppCommandBuffers[i]));
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

    std::lock_guard<std::mutex> holder{ m_lock };
    // Destroy all of the command buffers.
    for (auto i = 0U; i < count; ++i)
    {
        if (ppCommandBuffers[i])
        {
            m_pDevice->getHandle().freeCommandBuffers(getHandle(), 1, &ppCommandBuffers[i]->getHandle());
            m_allocatedCommandBuffers.erase(ppCommandBuffers[i]);
            m_commandBufferPool.free(ppCommandBuffers[i]);
        }
    }
}

void CommandPool::trim()
{
    std::lock_guard<std::mutex> holder{ m_lock };
    m_pDevice->getHandle().trimCommandPool(getHandle(), {});
}

void CommandPool::reset(bool freeMemory)
{
    std::lock_guard<std::mutex> holder{ m_lock };
    auto deviceHandle = m_pDevice->getHandle();
    ::vk::CommandPoolResetFlagBits flags = {};
    if (freeMemory)
    {
        flags = ::vk::CommandPoolResetFlagBits::eReleaseResources;
        for (CommandBuffer* cmd : m_allocatedCommandBuffers)
        {
            deviceHandle.freeCommandBuffers(getHandle(), 1, &cmd->getHandle());
            m_commandBufferPool.free(cmd);
        }
        m_allocatedCommandBuffers.clear();
        m_commandBufferPool.clear();
    }
    // TODO free after reset?
    deviceHandle.resetCommandPool(getHandle(), flags);
}
} // namespace aph::vk
