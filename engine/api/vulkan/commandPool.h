#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct CommandPoolCreateInfo
{
    Queue* queue = {};
    bool transient = { false };
};

class CommandPool : public ResourceHandle<::vk::CommandPool, CommandPoolCreateInfo>
{
public:
    CommandPool(Device* pDevice, const CreateInfoType& createInfo, HandleType pool);
    ~CommandPool();

    CommandBuffer* allocate();
    Result allocate(uint32_t count, CommandBuffer** ppCommandBuffers);
    void free(uint32_t count, CommandBuffer** ppCommandBuffers);
    void reset(bool freeMemory = false);
    void trim();

private:
    Device* m_pDevice = {};
    Queue* m_pQueue = {};
    HashSet<CommandBuffer*> m_allocatedCommandBuffers = {};
    ThreadSafeObjectPool<CommandBuffer> m_commandBufferPool;
    std::mutex m_lock;
};

} // namespace aph::vk
