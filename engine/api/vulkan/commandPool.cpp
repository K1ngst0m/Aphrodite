#include "commandPool.h"
#include "queue.h"
#include "device.h"

namespace aph::vk
{

void CommandPoolAllocator::allocate(uint32_t count, CommandBuffer** ppCommandBuffers, Queue* pQueue)
{
    // Queue* queue       = pQueue;
    // auto   deviceTable = m_pDevice->getDeviceTable();

    // // get command pool
    // VkCommandPool pool = {};
    // {
    //     auto queueIndices = queue->getFamilyIndex();

    //     if(m_commandPools.contains(queueIndices))
    //     {
    //         pool = m_commandPools.at(queueIndices);
    //     }
    //     else
    //     {
    //         CommandPoolCreateInfo   createInfo{.queue = queue};
    //         VkCommandPoolCreateInfo cmdPoolInfo{
    //             .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    //             .queueFamilyIndex = createInfo.queue->getFamilyIndex(),
    //         };

    //         if(createInfo.transient)
    //         {
    //             cmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    //         }

    //         _VR(deviceTable->vkCreateCommandPool(m_pDevice->getHandle(), &cmdPoolInfo, vkAllocator(), &pool));
    //         m_commandPools[queueIndices] = pool;
    //     }
    // }
}

void CommandPoolAllocator::allocateThread(uint32_t count, CommandBuffer** ppCommandBuffers, Queue* pQueue)
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
    _VR(m_pDevice->getDeviceTable()->vkAllocateCommandBuffers(m_pDevice->getHandle(), &allocInfo, handles.data()));

    for(auto i = 0; i < count; i++)
    {
        ppCommandBuffers[i] = m_commandBufferPool.allocate(m_pDevice, getHandle(), handles[i], m_pQueue);
        m_allocatedCommandBuffers.push_back(ppCommandBuffers[i]);
    }
    return Result::Success;
}
void CommandPool::free(uint32_t count, CommandBuffer** ppCommandBuffers)
{
    APH_ASSERT(ppCommandBuffers);

    // Destroy all of the command buffers.
    for(auto i = 0U; i < count; ++i)
    {
        if(ppCommandBuffers[i])
        {
            m_commandBufferPool.free(ppCommandBuffers[i]);
        }
    }
}

CommandPool::~CommandPool() = default;
}  // namespace aph::vk
