#include "queue.h"
#include "device.h"
#include <mutex>

namespace aph::vk
{

Queue::Queue(HandleType handle, uint32_t queueFamilyIndex, uint32_t index, QueueType type) :
    ResourceHandle(handle),
    m_queueFamilyIndex(queueFamilyIndex),
    m_index(index),
    m_type(type)
{
}

Result Queue::submit(const std::vector<QueueSubmitInfo>& submitInfos, Fence* pFence)
{
    SmallVector<SmallVector<::vk::CommandBuffer>>      vkCmds2D;
    SmallVector<SmallVector<::vk::PipelineStageFlags>> vkWaitStages2D;
    SmallVector<SmallVector<::vk::Semaphore>>          vkWaitSemaphores2D;
    SmallVector<SmallVector<::vk::Semaphore>>          vkSignalSemaphores2D;

    std::vector<::vk::SubmitInfo> vkSubmits;
    for(const auto& submitInfo : submitInfos)
    {
        SmallVector<::vk::CommandBuffer>      vkCmds;
        SmallVector<::vk::PipelineStageFlags> vkWaitStages;
        SmallVector<::vk::Semaphore>          vkWaitSemaphores;
        SmallVector<::vk::Semaphore>          vkSignalSemaphores;

        for(auto* cmd : submitInfo.commandBuffers)
        {
            vkCmds.push_back(cmd->getHandle());
        }

        for(auto* sem : submitInfo.waitSemaphores)
        {
            vkWaitSemaphores.push_back(sem->getHandle());
        }

        for(auto* sem : submitInfo.signalSemaphores)
        {
            vkSignalSemaphores.push_back(sem->getHandle());
        }

        ::vk::SubmitInfo info{};
        info.setCommandBuffers(vkCmds).setWaitSemaphores(vkWaitSemaphores).setSignalSemaphores(vkSignalSemaphores);

        if(submitInfo.waitStages.empty())
        {
            if(vkWaitStages.empty())
            {
                vkWaitStages.resize(submitInfo.waitSemaphores.size(), ::vk::PipelineStageFlagBits::eAllCommands);
            }
            info.pWaitDstStageMask = vkWaitStages.data();
        }
        vkCmds2D.push_back(std::move(vkCmds));
        vkWaitStages2D.push_back(std::move(vkWaitStages));
        vkWaitSemaphores2D.push_back(std::move(vkWaitSemaphores));
        vkSignalSemaphores2D.push_back(std::move(vkSignalSemaphores));
        vkSubmits.push_back(info);
    }

    std::lock_guard<std::mutex> lock{m_lock};
    auto                        result = getHandle().submit(vkSubmits, pFence ? pFence->getHandle() : VK_NULL_HANDLE);
    return utils::getResult(result);
}

Result Queue::waitIdle()
{
    std::lock_guard<std::mutex> holder{m_lock};
    return utils::getResult(getHandle().waitIdle());
}

Result Queue::present(const ::vk::PresentInfoKHR& presentInfo)
{
    std::lock_guard<std::mutex> lock{m_lock};
    auto                        result = getHandle().presentKHR(presentInfo);
    return utils::getResult(result);
}
}  // namespace aph::vk
