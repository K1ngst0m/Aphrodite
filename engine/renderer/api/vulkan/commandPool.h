#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "common/spinlock.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;

class VulkanCommandPool : public ResourceHandle<VkCommandPool>
{
public:
    static VulkanCommandPool *Create(VulkanDevice *device, uint32_t queueFamilyIndex, VkCommandPool pool);
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, VkCommandBuffer *pCommandBuffers);
    void freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers);
    uint32_t getQueueFamilyIndex() const;

private:
    VulkanDevice *m_device = nullptr;
    uint32_t m_queueFamilyIndex = 0;
    SpinLock m_spinLock;
};

using QueueFamilyCommandPools = std::unordered_map<uint32_t, VulkanCommandPool *>;
}  // namespace vkl

#endif  // COMMANDPOOL_H_
