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

struct QueueSubmitInfo2
{
    std::vector<VkCommandBufferSubmitInfo> commands;
    std::vector<VkSemaphoreSubmitInfo>     waits;
    std::vector<VkSemaphoreSubmitInfo>     signals;
};

class Queue : public ResourceHandle<VkQueue>
{
public:
    Queue(VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& properties);

    uint32_t     getFamilyIndex() const { return m_queueFamilyIndex; }
    uint32_t     getIndex() const { return m_index; }
    VkQueueFlags getFlags() const { return m_properties.queueFlags; }
    QueueType    getType() const { return m_type; }
    VkResult     waitIdle() { return vkQueueWaitIdle(getHandle()); }
    VkResult     submit(const std::vector<QueueSubmitInfo>& submitInfos, VkFence fence);
    VkResult     submit(const std::vector<QueueSubmitInfo2>& submitInfos);

private:
    uint32_t                m_queueFamilyIndex = {};
    uint32_t                m_index            = {};
    VkQueueFamilyProperties m_properties       = {};
    QueueType               m_type             = {};
};

using QueueFamily = std::vector<std::unique_ptr<Queue>>;

}  // namespace aph::vk

#endif  // QUEUE_H_
