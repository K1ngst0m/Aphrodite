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

    Result allocate(uint32_t count, CommandBuffer** ppCommandBuffers);
    void   free(uint32_t count, CommandBuffer** ppCommandBuffers);

    bool isOnRecord() const { return m_onRecord; }

private:
    Device*                             m_pDevice                 = {};
    Queue*                              m_pQueue                  = {};
    bool                                m_onRecord                = {};
    std::vector<CommandBuffer*>         m_allocatedCommandBuffers = {};
    ThreadSafeObjectPool<CommandBuffer> m_commandBufferPool;
};

}  // namespace aph::vk

#endif  // COMMANDPOOL_H_
