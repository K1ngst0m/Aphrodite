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

    auto getFamilyIndex() const -> uint32_t;
    auto getIndex() const -> uint32_t;
    auto getType() const -> QueueType;
    auto waitIdle() -> Result;
    auto submit(ArrayProxy<QueueSubmitInfo> submitInfos, Fence* pFence = nullptr) -> Result;
    auto present(const ::vk::PresentInfoKHR& presentInfo) -> Result;

private:
    std::mutex m_lock           = {};
    uint32_t m_queueFamilyIndex = {};
    uint32_t m_index            = {};
    QueueType m_type            = {};
};

} // namespace aph::vk
