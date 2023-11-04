#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "vkUtils.h"

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
    CommandPool(Device* pDevice, const CreateInfoType& createInfo, HandleType pool) :
        ResourceHandle(pool, createInfo),
        m_pDevice(pDevice),
        m_pQueue(createInfo.queue)
    {
    }
    ~CommandPool();

    CommandBuffer* allocate();
    Result allocate(uint32_t count, CommandBuffer** ppCommandBuffers);
    void   free(uint32_t count, CommandBuffer** ppCommandBuffers);
    void   reset();

    bool isOnRecord() const { return m_onRecord; }

private:
    Device*                             m_pDevice                 = {};
    Queue*                              m_pQueue                  = {};
    bool                                m_onRecord                = {};
    std::vector<CommandBuffer*>         m_allocatedCommandBuffers = {};
    ThreadSafeObjectPool<CommandBuffer> m_commandBufferPool;
};

class CommandPoolAllocator
{
public:
    CommandPoolAllocator(Device* pDevice) : m_pDevice(pDevice) {}
    void clear();

    Result acquire(const CommandPoolCreateInfo& createInfo, uint32_t count, CommandPool** ppCommandPool);
    void   release(uint32_t count, CommandPool** ppCommandPool);

private:
    Device*                                                 m_pDevice        = {};
    std::unordered_map<QueueType, std::set<CommandPool*>>   m_allPools       = {};
    std::unordered_map<QueueType, std::queue<CommandPool*>> m_availablePools = {};
    ThreadSafeObjectPool<CommandPool>                       m_resourcePool   = {};
    std::mutex                                              m_lock           = {};
};

}  // namespace aph::vk

#endif  // COMMANDPOOL_H_
