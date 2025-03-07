#pragma once

#include "common/arrayProxy.h"
#include "vkUtils.h"

namespace aph::vk
{
class CommandBuffer;
class Semaphore;
class Fence;

struct QueueSubmitInfo
{
    std::vector<CommandBuffer*> commandBuffers;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<Semaphore*> waitSemaphores;
    std::vector<Semaphore*> signalSemaphores;
};

class Queue : public ResourceHandle<::vk::Queue>
{
public:
    Queue(HandleType handle, uint32_t queueFamilyIndex, uint32_t index, QueueType type);

    uint32_t getFamilyIndex() const
    {
        return m_queueFamilyIndex;
    }
    uint32_t getIndex() const
    {
        return m_index;
    }
    QueueType getType() const
    {
        return m_type;
    }
    Result waitIdle();
    Result submit(ArrayProxy<QueueSubmitInfo> submitInfos, Fence* pFence = nullptr);
    Result present(const ::vk::PresentInfoKHR& presentInfo);

private:
    std::mutex m_lock = {};
    uint32_t m_queueFamilyIndex = {};
    uint32_t m_index = {};
    QueueType m_type = {};
};

} // namespace aph::vk
