#pragma once

#include "allocator/objectPool.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace aph::vk
{

enum class CommandPoolResetFlag
{
    None = 0,
    ReleaseResources = 1
};

enum class CommandBufferUsage
{
    OneTime, // Reset and free after submission
    Reusable, // Reset but don't free after submission
    Persistent // Don't reset or free after submission
};

// Thread ID wrapper to identify threads
class ThreadId
{
public:
    ThreadId()
        : m_id(std::this_thread::get_id())
    {
    }
    bool operator==(const ThreadId& other) const
    {
        return m_id == other.m_id;
    }

    struct Hash
    {
        std::size_t operator()(const ThreadId& id) const noexcept
        {
            return std::hash<std::thread::id>{}(id.m_id);
        }
    };

private:
    std::thread::id m_id;
};

// CommandBufferPool for a specific thread
class ThreadCommandPool
{
public:
    ThreadCommandPool(Device* pDevice, Queue* pQueue, bool transient = false);
    ~ThreadCommandPool();

    CommandBuffer* acquireCommandBuffer(CommandBufferUsage usage);
    void release(CommandBuffer* pCmdBuffer);
    void reset(CommandPoolResetFlag flags = CommandPoolResetFlag::None);
    void trim();

private:
    CommandBuffer* allocate();
    Result allocate(uint32_t count, CommandBuffer** ppCommandBuffers);
    void free(uint32_t count, CommandBuffer** ppCommandBuffers);

private:
    Device* m_pDevice = {};
    Queue* m_pQueue = {};
    ::vk::CommandPool m_commandPool = {};
    bool m_transient = false;

    // Command buffers currently in use
    HashSet<CommandBuffer*> m_activeCommandBuffers = {};
    // Command buffers waiting to be reused
    SmallVector<CommandBuffer*> m_availableCommandBuffers = {};
    ThreadSafeObjectPool<CommandBuffer> m_commandBufferPool;
    std::mutex m_poolMutex;
};

// Manages thread-safe command buffer allocation
class CommandBufferAllocator
{
public:
    CommandBufferAllocator(Device* pDevice);
    ~CommandBufferAllocator();

    // Acquire a command buffer for the current thread from the appropriate queue
    CommandBuffer* acquire(QueueType queueType, CommandBufferUsage usage = CommandBufferUsage::OneTime);

    // Release a command buffer back to the allocator
    void release(CommandBuffer* pCmdBuffer);

    // Reset all command pools
    void reset();

    // Get the number of active command buffers
    size_t getActiveCommandBufferCount() const;

private:
    // Get or create a thread command pool for the current thread
    ThreadCommandPool* getThreadCommandPool(QueueType queueType);

private:
    Device* m_pDevice = {};

    // Maps thread ID to command pools for different queue types
    using ThreadPoolMap = HashMap<ThreadId, HashMap<QueueType, std::unique_ptr<ThreadCommandPool>>, ThreadId::Hash>;
    ThreadPoolMap m_threadPools;

    // Track total active command buffers across all threads
    std::atomic<size_t> m_activeCommandBufferCount = 0;

    // Guards thread pool creation
    mutable std::mutex m_threadPoolMutex;
};

} // namespace aph::vk
