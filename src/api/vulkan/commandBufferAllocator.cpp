#include "commandBufferAllocator.h"
#include "device.h"

namespace aph::vk
{

ThreadCommandPool::ThreadCommandPool(Device* pDevice, Queue* pQueue, bool transient)
    : m_pDevice(pDevice)
{
    APH_PROFILER_SCOPE();
    CommandPoolCreateInfo createInfo;
    createInfo.queue = pQueue;
    createInfo.transient = transient;
    APH_VR(pDevice->create(createInfo, &m_pCommandPool));
}

ThreadCommandPool::~ThreadCommandPool()
{
    APH_PROFILER_SCOPE();
    if (m_pCommandPool)
    {
        m_pDevice->destroy(m_pCommandPool);
        m_pCommandPool = nullptr;
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
        APH_VR(pCmdBuffer->reset());
    }
    else
    {
        // Allocate a new command buffer if none are available
        pCmdBuffer = m_pCommandPool->allocate();
    }

    // Mark this command buffer as active
    m_activeCommandBuffers.insert(pCmdBuffer);
    return pCmdBuffer;
}

void ThreadCommandPool::releaseCommandBuffer(CommandBuffer* pCmdBuffer)
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

void ThreadCommandPool::reset()
{
    APH_PROFILER_SCOPE();
    std::lock_guard<std::mutex> guard(m_poolMutex);

    // Reset the command pool to release all command buffers
    m_pCommandPool->reset(CommandPoolResetFlag::ReleaseResources);

    // Clear tracking collections since command buffers were released
    m_activeCommandBuffers.clear();
    m_availableCommandBuffers.clear();
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
        pThreadPool->releaseCommandBuffer(pCmdBuffer);
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
