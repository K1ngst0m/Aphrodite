#include "commandBufferAllocator.h"
#include "device.h"

namespace aph::vk
{

ThreadCommandPool::ThreadCommandPool(Device* pDevice, Queue* pQueue, bool transient)
    : m_pDevice(pDevice)
    , m_pQueue(pQueue)
    , m_transient(transient)
{
    APH_PROFILER_SCOPE();

    ::vk::CommandPoolCreateInfo vkCreateInfo{};
    vkCreateInfo.setQueueFamilyIndex(pQueue->getFamilyIndex());
    if (transient)
    {
        vkCreateInfo.setFlags(::vk::CommandPoolCreateFlagBits::eTransient);
    }
    vkCreateInfo.setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    auto [result, pool] = pDevice->getHandle().createCommandPool(vkCreateInfo, vk_allocator());
    VK_VR(result);

    m_commandPool = pool;
}

ThreadCommandPool::~ThreadCommandPool()
{
    APH_PROFILER_SCOPE();
    if (m_commandPool)
    {
        // Free all command buffers
        reset(CommandPoolResetFlag::ReleaseResources);
        m_pDevice->getHandle().destroyCommandPool(m_commandPool, vk_allocator());
        m_commandPool = nullptr;
    }
}

CommandBuffer* ThreadCommandPool::acquireCommandBuffer(CommandBufferUsage usage)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> guard(m_poolMutex);

    CommandBuffer* pCmdBuffer = nullptr;

    // Try to reuse an available command buffer if possible
    if (!m_availableCommandBuffers.empty())
    {
        pCmdBuffer = m_availableCommandBuffers.back();
        m_availableCommandBuffers.pop_back();
        APH_VERIFY_RESULT(pCmdBuffer->reset());
    }
    else
    {
        // Allocate a new command buffer if none are available
        pCmdBuffer = allocate();
    }

    // Mark this command buffer as active
    m_activeCommandBuffers.insert(pCmdBuffer);
    return pCmdBuffer;
}

void ThreadCommandPool::release(CommandBuffer* pCmdBuffer)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> guard(m_poolMutex);

    // Make sure this command buffer belongs to this pool
    if (m_activeCommandBuffers.contains(pCmdBuffer))
    {
        m_activeCommandBuffers.erase(pCmdBuffer);
        m_availableCommandBuffers.push_back(pCmdBuffer);
    }
    else
    {
        CM_LOG_ERR("Attempted to release command buffer that doesn't belong to this pool");
    }
}

void ThreadCommandPool::reset(CommandPoolResetFlag flags)
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> guard(m_poolMutex);

    auto deviceHandle = m_pDevice->getHandle();
    ::vk::CommandPoolResetFlagBits vkFlags = {};

    if (flags == CommandPoolResetFlag::ReleaseResources)
    {
        vkFlags = ::vk::CommandPoolResetFlagBits::eReleaseResources;
        // Free all the command buffers
        for (CommandBuffer* cmd : m_activeCommandBuffers)
        {
            deviceHandle.freeCommandBuffers(m_commandPool, 1, &cmd->getHandle());
            m_commandBufferPool.free(cmd);
        }
        m_activeCommandBuffers.clear();

        // Also clear the available command buffers
        for (CommandBuffer* cmd : m_availableCommandBuffers)
        {
            deviceHandle.freeCommandBuffers(m_commandPool, 1, &cmd->getHandle());
            m_commandBufferPool.free(cmd);
        }
        m_availableCommandBuffers.clear();
        m_commandBufferPool.clear();
    }

    deviceHandle.resetCommandPool(m_commandPool, vkFlags);
}

void ThreadCommandPool::trim()
{
    APH_PROFILER_SCOPE();
    m_pDevice->getHandle().trimCommandPool(m_commandPool, {});
}

CommandBuffer* ThreadCommandPool::allocate()
{
    APH_PROFILER_SCOPE();
    CommandBuffer* pCmd = {};
    APH_VERIFY_RESULT(allocate(1, &pCmd));
    return pCmd;
}

Result ThreadCommandPool::allocate(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    APH_PROFILER_SCOPE();

    // Allocate new command buffers
    ::vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.setCommandPool(m_commandPool).setLevel(::vk::CommandBufferLevel::ePrimary).setCommandBufferCount(count);

    auto [result, handles] = m_pDevice->getHandle().allocateCommandBuffers(allocInfo);
    if (result != ::vk::Result::eSuccess)
    {
        return Result::RuntimeError;
    }

    for (auto i = 0; i < count; i++)
    {
        ppCommandBuffers[i] = m_commandBufferPool.allocate(m_pDevice, handles[i], m_pQueue, m_transient);
        APH_ASSERT(!m_activeCommandBuffers.contains(ppCommandBuffers[i]));
    }

    return Result::Success;
}

void ThreadCommandPool::free(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(ppCommandBuffers);

    // Free the specified command buffers
    for (auto i = 0U; i < count; ++i)
    {
        if (ppCommandBuffers[i])
        {
            m_pDevice->getHandle().freeCommandBuffers(m_commandPool, 1, &ppCommandBuffers[i]->getHandle());
            m_activeCommandBuffers.erase(ppCommandBuffers[i]);
            m_commandBufferPool.free(ppCommandBuffers[i]);
        }
    }
}

CommandBufferAllocator::CommandBufferAllocator(Device* pDevice)
    : m_pDevice(pDevice)
{
    APH_PROFILER_SCOPE();
}

CommandBufferAllocator::~CommandBufferAllocator()
{
    APH_PROFILER_SCOPE();
    reset();
}

CommandBuffer* CommandBufferAllocator::acquire(QueueType queueType, CommandBufferUsage usage)
{
    APH_PROFILER_SCOPE();

    ThreadCommandPool* pThreadPool = getThreadCommandPool(queueType);
    if (!pThreadPool)
    {
        CM_LOG_ERR("Failed to get thread command pool for queue type %d", static_cast<int>(queueType));
        return nullptr;
    }

    CommandBuffer* pCmdBuffer = pThreadPool->acquireCommandBuffer(usage);
    if (pCmdBuffer)
    {
        m_activeCommandBufferCount++;
    }

    return pCmdBuffer;
}

void CommandBufferAllocator::release(CommandBuffer* pCmdBuffer)
{
    APH_PROFILER_SCOPE();

    if (!pCmdBuffer)
    {
        return;
    }

    // Each command buffer knows its queue
    Queue* pQueue = pCmdBuffer->m_pQueue;
    if (!pQueue)
    {
        CM_LOG_ERR("Command buffer has no associated queue");
        return;
    }

    // Get the thread pool matching this queue type
    ThreadCommandPool* pThreadPool = getThreadCommandPool(pQueue->getType());
    if (pThreadPool)
    {
        pThreadPool->release(pCmdBuffer);
        m_activeCommandBufferCount--;
    }
}

void CommandBufferAllocator::reset()
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> guard(m_threadPoolMutex);

    // Reset each thread's command pools
    for (auto& [threadId, queuePools] : m_threadPools)
    {
        for (auto& [queueType, threadPool] : queuePools)
        {
            threadPool->reset();
        }
    }

    m_activeCommandBufferCount = 0;
}

size_t CommandBufferAllocator::getActiveCommandBufferCount() const
{
    return m_activeCommandBufferCount;
}

ThreadCommandPool* CommandBufferAllocator::getThreadCommandPool(QueueType queueType)
{
    APH_PROFILER_SCOPE();

    ThreadId currentThreadId;
    std::lock_guard<std::mutex> guard(m_threadPoolMutex);

    // Get or create the map of queue pools for this thread
    auto& queuePools = m_threadPools[currentThreadId];

    // Check if we already have a pool for this queue type
    auto it = queuePools.find(queueType);
    if (it != queuePools.end())
    {
        return it->second.get();
    }

    // Create a new thread command pool for this queue type
    Queue* pQueue = m_pDevice->getQueue(queueType);
    if (!pQueue)
    {
        CM_LOG_ERR("Failed to get queue of type %d", static_cast<int>(queueType));
        return nullptr;
    }

    auto newPool = std::make_unique<ThreadCommandPool>(m_pDevice, pQueue);
    ThreadCommandPool* pThreadPool = newPool.get();
    queuePools[queueType] = std::move(newPool);

    return pThreadPool;
}

} // namespace aph::vk
