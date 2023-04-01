#ifndef QUEUE_H_
#define QUEUE_H_

#include "device.h"

namespace vkl
{
class VulkanQueue : public ResourceHandle<VkQueue>
{
public:
    VulkanQueue(VulkanDevice *device, VkQueue queue, uint32_t queueFamilyIndex, uint32_t index,
                const VkQueueFamilyProperties &propertiesd);

    uint32_t getFamilyIndex() const { return m_queueFamilyIndex; }
    uint32_t getIndex() const { return m_index; }
    VkQueueFlags getFlags() const { return m_properties.queueFlags; }
    VkResult waitIdle() { return VkResult(vkQueueWaitIdle(getHandle())); }

    VkResult submit(uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence);
    VkResult present(const VkPresentInfoKHR &presentInfo);

private:
    VkResult acquireCommandBuffer(VulkanCommandBuffer **pCommandBuffer);

    VulkanDevice *m_device = nullptr;

    uint32_t m_queueFamilyIndex = 0;
    uint32_t m_index = 0;
    VkQueueFamilyProperties m_properties;
};
}  // namespace vkl

#endif  // QUEUE_H_
