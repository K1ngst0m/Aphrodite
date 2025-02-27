#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"
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

class CommandPool : public ResourceHandle<::vk::CommandPool, CommandPoolCreateInfo>
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

}  // namespace aph::vk

#endif  // COMMANDPOOL_H_
