#ifndef QUEUE_H_
#define QUEUE_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;
class VulkanCommandBuffer;

struct QueueSubmitInfo{
    std::vector<VulkanCommandBuffer*> commandBuffers;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkSemaphore> signalSemaphores;
};

class VulkanQueue : public ResourceHandle<VkQueue>
{
public:
    VulkanQueue(VulkanDevice *device, VkQueue queue, uint32_t queueFamilyIndex, uint32_t index,
                const VkQueueFamilyProperties &properties);

    uint32_t getFamilyIndex() const { return m_queueFamilyIndex; }
    uint32_t getIndex() const { return m_index; }
    VkQueueFlags getFlags() const { return m_properties.queueFlags; }
    VkResult waitIdle() { return VkResult(vkQueueWaitIdle(getHandle())); }

    VkResult submit(const std::vector<QueueSubmitInfo>& submitInfos, VkFence fence);

private:
    VkResult acquireCommandBuffer(VulkanCommandBuffer **pCommandBuffer);

    VulkanDevice *m_device = nullptr;

    uint32_t m_queueFamilyIndex = 0;
    uint32_t m_index = 0;
    VkQueueFamilyProperties m_properties;
};

using QueueFamily = std::vector<VulkanQueue *>;

}  // namespace aph

#endif  // QUEUE_H_
