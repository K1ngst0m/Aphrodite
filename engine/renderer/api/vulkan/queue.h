#ifndef QUEUE_H_
#define QUEUE_H_

#include "device.h"

namespace vkl {
struct PresentInfo {
    VkPipelineStageFlags *pWaitDstStageMask;
    uint32_t              signalSemaphoreCount;
    VkSemaphore          *pSignalSemaphores;
    uint32_t              waitSemaphoreCount;
    VkSemaphore          *pWaitSemaphores;
    uint32_t              swapchainCount;
    const uint32_t       *pImageIndices;
    VkResult             *pResults;
    VulkanSwapChain     **pSwapchains;
};

class VulkanQueue : public ResourceHandle<VkQueue> {
public:
    VulkanQueue(VulkanDevice *device, VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties &propertiesd);

    uint32_t     getFamilyIndex() const;
    uint32_t     getIndex() const;
    VkQueueFlags getFlags() const;
    VkResult     submit(uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence);
    VkResult     present(const PresentInfo *pPresentInfo);
    VkResult     waitIdle();

private:
    VkResult acquireCommandBuffer(VulkanCommandBuffer **pCommandBuffer);

    VulkanDevice *m_device = nullptr;

    uint32_t                m_queueFamilyIndex = 0;
    uint32_t                m_index            = 0;
    VkQueueFamilyProperties m_properties;
};
} // namespace vkl

#endif // QUEUE_H_
