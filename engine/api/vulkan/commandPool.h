#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "threads/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

struct CommandPoolCreateInfo
{
    uint32_t                 queueFamilyIndex = {};
    VkCommandPoolCreateFlags flags            = {VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
};

class CommandPool : public ResourceHandle<VkCommandPool, CommandPoolCreateInfo>
{
public:
    CommandPool(const CommandPoolCreateInfo& createInfo, Device* device, VkCommandPool pool);
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, VkCommandBuffer* pCommandBuffers);
    void     freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
    uint32_t getQueueFamilyIndex() const;

private:
    Device*  m_device   = {};
    SpinLock m_spinLock = {};
};

using QueueFamilyCommandPools = std::unordered_map<uint32_t, CommandPool*>;
}  // namespace aph::vk

#endif  // COMMANDPOOL_H_
