#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "common/spinlock.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;

struct CommandPoolCreateInfo{
    uint32_t queueFamilyIndex;
    VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
};

class VulkanCommandPool : public ResourceHandle<VkCommandPool, CommandPoolCreateInfo>
{
public:
    VulkanCommandPool(const CommandPoolCreateInfo& createInfo, VulkanDevice *device, VkCommandPool pool);
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, VkCommandBuffer *pCommandBuffers);
    void freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers);
    uint32_t getQueueFamilyIndex() const;

private:
    VulkanDevice *m_device = nullptr;
    SpinLock m_spinLock;
};

using QueueFamilyCommandPools = std::unordered_map<uint32_t, VulkanCommandPool *>;
}  // namespace aph

#endif  // COMMANDPOOL_H_
