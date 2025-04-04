#include "queue.h"
#include "device.h"

#include <mutex>

namespace aph::vk
{

Queue::Queue(HandleType handle, uint32_t queueFamilyIndex, uint32_t index, QueueType type)
    : ResourceHandle(handle)
    , m_queueFamilyIndex(queueFamilyIndex)
    , m_index(index)
    , m_type(type)
{
}

Result Queue::submit(ArrayProxy<QueueSubmitInfo> submitInfos, Fence* pFence)
{
    SmallVector<::vk::SubmitInfo> vkSubmits;
    vkSubmits.reserve(submitInfos.size());

    // These vectors store the data for each submit info to ensure it stays valid until submission
    SmallVector<SmallVector<::vk::CommandBuffer>> vkCmds;
    SmallVector<SmallVector<::vk::Semaphore>> vkWaitSemaphores;
    SmallVector<SmallVector<::vk::Semaphore>> vkSignalSemaphores;
    SmallVector<SmallVector<::vk::PipelineStageFlags>> vkWaitStages;

    vkCmds.reserve(submitInfos.size());
    vkWaitSemaphores.reserve(submitInfos.size());
    vkSignalSemaphores.reserve(submitInfos.size());
    vkWaitStages.reserve(submitInfos.size());

    for (const auto& submitInfo : submitInfos)
    {
        ::vk::SubmitInfo info{};

        // Process command buffers
        auto& cmds = vkCmds.emplace_back();
        cmds.reserve(submitInfo.commandBuffers.size());
        for (auto* cmd : submitInfo.commandBuffers)
        {
            cmds.push_back(cmd->getHandle());
        }
        info.setCommandBuffers(cmds);

        // Process wait semaphores
        auto& waitSemaphores = vkWaitSemaphores.emplace_back();
        waitSemaphores.reserve(submitInfo.waitSemaphores.size());
        for (auto* sem : submitInfo.waitSemaphores)
        {
            waitSemaphores.push_back(sem->getHandle());
        }
        info.setWaitSemaphores(waitSemaphores);

        // Process signal semaphores
        auto& signalSemaphores = vkSignalSemaphores.emplace_back();
        signalSemaphores.reserve(submitInfo.signalSemaphores.size());
        for (auto* sem : submitInfo.signalSemaphores)
        {
            signalSemaphores.push_back(sem->getHandle());
        }
        info.setSignalSemaphores(signalSemaphores);

        // Set wait stages if needed
        if (!submitInfo.waitStages.empty())
        {
            auto& waitStages = vkWaitStages.emplace_back();
            waitStages.reserve(submitInfo.waitStages.size());
            for (auto stage : submitInfo.waitStages)
            {
                waitStages.push_back(static_cast<::vk::PipelineStageFlags>(stage));
            }
            info.setWaitDstStageMask(waitStages);
        }
        else if (!waitSemaphores.empty())
        {
            auto& waitStages = vkWaitStages.emplace_back();
            waitStages.resize(waitSemaphores.size(),
                              ::vk::PipelineStageFlags(::vk::PipelineStageFlagBits::eAllCommands));
            info.setWaitDstStageMask(waitStages);
        }

        vkSubmits.push_back(std::move(info));
    }

    std::lock_guard<std::mutex> lock{m_lock};
    auto result = getHandle().submit(vkSubmits, pFence ? pFence->getHandle() : VK_NULL_HANDLE);
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
    auto result = getHandle().presentKHR(presentInfo);
    return utils::getResult(result);
}
} // namespace aph::vk
