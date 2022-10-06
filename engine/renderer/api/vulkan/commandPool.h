#ifndef COMMANDPOOL_H_
#define COMMANDPOOL_H_

#include "device.h"
#include "common/spinlock.h"

namespace vkl {
class VulkanCommandPool : public ResourceHandle<VkCommandPool> {
public:
    static VulkanCommandPool* Create(VulkanDevice *device, uint32_t queueFamilyIndex, VkCommandPool pool);
    VkResult        allocateCommandBuffers(uint32_t commandBufferCount, VkCommandBuffer *pCommandBuffers);
    void            freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers);
    uint32_t        getQueueFamilyIndex() const;

private:
    VulkanDevice *_device           = nullptr;
    uint32_t      _queueFamilyIndex = 0;
    SpinLock      _spinLock;
};
} // namespace vkl

#endif // COMMANDPOOL_H_
