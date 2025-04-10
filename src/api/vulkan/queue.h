#pragma once

#include "common/arrayProxy.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct QueueSubmitInfo
{
    SmallVector<CommandBuffer*> commandBuffers;
    SmallVector<VkPipelineStageFlags> waitStages;
    SmallVector<Semaphore*> waitSemaphores;
    SmallVector<Semaphore*> signalSemaphores;
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
    std::mutex m_lock           = {};
    uint32_t m_queueFamilyIndex = {};
    uint32_t m_index            = {};
    QueueType m_type            = {};
};

} // namespace aph::vk
