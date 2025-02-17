#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include <volk.h>
#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"

namespace aph::vk
{
class Device;
class CommandBuffer;
class Queue;

struct CommandPoolCreateInfo
{
    Queue* queue     = {};
    bool   transient = {false};
};

class CommandPool : public ResourceHandle<VkCommandPool, CommandPoolCreateInfo>
{
public:
    CommandPool(Device* pDevice, const CreateInfoType& createInfo, HandleType pool);
    ~CommandPool();

    CommandBuffer* allocate();
    Result         allocate(uint32_t count, CommandBuffer** ppCommandBuffers);
    void           free(uint32_t count, CommandBuffer** ppCommandBuffers);
    void           reset(bool freeMemory = false);
    void           trim();

    bool isOnRecord() const { return m_onRecord; }

private:
    Device*                             m_pDevice                 = {};
    Queue*                              m_pQueue                  = {};
    bool                                m_onRecord                = {};
    HashSet<CommandBuffer*>             m_allocatedCommandBuffers = {};
    ThreadSafeObjectPool<CommandBuffer> m_commandBufferPool;
    std::mutex                          m_lock;
};

class CommandPoolAllocator
{
public:
    CommandPoolAllocator(Device* pDevice) : m_pDevice(pDevice) {}
    void clear();

    Result acquire(const CommandPoolCreateInfo& createInfo, uint32_t count, CommandPool** ppCommandPool);
    void   release(uint32_t count, CommandPool** ppCommandPool);

private:
    Device*                                      m_pDevice        = {};
    HashMap<QueueType, std::set<CommandPool*>>   m_allPools       = {};
    HashMap<QueueType, std::queue<CommandPool*>> m_availablePools = {};
    ThreadSafeObjectPool<CommandPool>            m_resourcePool   = {};
    std::mutex                                   m_lock           = {};
};

}  // namespace aph::vk

#endif  // COMMANDPOOL_H_
