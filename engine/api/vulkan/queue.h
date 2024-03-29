#ifndef QUEUE_H_
#define QUEUE_H_

#include "vkUtils.h"

namespace aph::vk
{
class Device;
class CommandBuffer;
class Semaphore;
class Fence;

struct QueueSubmitInfo
{
    std::vector<CommandBuffer*>       commandBuffers;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<Semaphore*>           waitSemaphores;
    std::vector<Semaphore*>           signalSemaphores;
};

class Queue : public ResourceHandle<VkQueue>
{
public:
    Queue(Device* pDevice, HandleType handle, uint32_t queueFamilyIndex, uint32_t index,
          const VkQueueFamilyProperties& properties);

    uint32_t     getFamilyIndex() const { return m_queueFamilyIndex; }
    uint32_t     getIndex() const { return m_index; }
    VkQueueFlags getFlags() const { return m_properties.queueFlags; }
    QueueType    getType() const { return m_type; }
    Result       waitIdle();
    Result       submit(const std::vector<QueueSubmitInfo>& submitInfos, Fence* pFence = nullptr);
    Result       present(const VkPresentInfoKHR& presentInfo);

private:
    std::mutex              m_lock             = {};
    uint32_t                m_queueFamilyIndex = {};
    uint32_t                m_index            = {};
    VkQueueFamilyProperties m_properties       = {};
    QueueType               m_type             = {};
    Device*                 m_pDevice          = {};
};

}  // namespace aph::vk

#endif  // QUEUE_H_
