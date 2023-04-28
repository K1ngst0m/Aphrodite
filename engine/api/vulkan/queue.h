#ifndef QUEUE_H_
#define QUEUE_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class CommandBuffer;

struct QueueSubmitInfo
{
    std::vector<CommandBuffer*>       commandBuffers;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore>          waitSemaphores;
    std::vector<VkSemaphore>          signalSemaphores;
};

class Queue : public ResourceHandle<VkQueue>
{
public:
    Queue(VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& properties);

    uint32_t     getFamilyIndex() const { return m_queueFamilyIndex; }
    uint32_t     getIndex() const { return m_index; }
    VkQueueFlags getFlags() const { return m_properties.queueFlags; }
    VkResult     waitIdle() { return vkQueueWaitIdle(getHandle()); }
    VkResult     submit(const std::vector<QueueSubmitInfo>& submitInfos, VkFence fence);

private:
    uint32_t                m_queueFamilyIndex = {};
    uint32_t                m_index            = {};
    VkQueueFamilyProperties m_properties       = {};
};

using QueueFamily = std::vector<std::unique_ptr<Queue>>;

}  // namespace aph::vk

#endif  // QUEUE_H_
