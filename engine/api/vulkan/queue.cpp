#include "queue.h"
#include "device.h"

namespace aph::vk
{

Queue::Queue(HandleType handle, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& propertiesd) :
    ResourceHandle(handle),
    m_queueFamilyIndex(queueFamilyIndex),
    m_index(index),
    m_properties(propertiesd)
{
    if(m_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        m_type = QueueType::GRAPHICS;
    }
    else if(m_properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        m_type = QueueType::COMPUTE;
    }
    else if(m_properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        m_type = QueueType::TRANSFER;
    }
    else
    {
        APH_ASSERT(false);
    }
}

Result Queue::submit(const std::vector<QueueSubmitInfo>& submitInfos, Fence* pFence)
{
    std::vector<VkSubmitInfo>         vkSubmits;
    std::vector<VkCommandBuffer>      vkCmds;
    std::vector<VkPipelineStageFlags> vkWaitStages;
    std::vector<VkSemaphore>          vkWaitSemaphores;
    std::vector<VkSemaphore>          vkSignalSemaphores;

    for(const auto& submitInfo : submitInfos)
    {
        uint32_t cmdOffset = {static_cast<uint32_t>(vkCmds.size())};
        uint32_t cmdSize   = {static_cast<uint32_t>(submitInfo.commandBuffers.size())};

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

        VkSubmitInfo info{
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = static_cast<uint32_t>(submitInfo.waitSemaphores.size()),
            .pWaitSemaphores      = vkWaitSemaphores.data(),
            .pWaitDstStageMask    = submitInfo.waitStages.data(),
            .commandBufferCount   = cmdSize,
            .pCommandBuffers      = &vkCmds[cmdOffset],
            .signalSemaphoreCount = static_cast<uint32_t>(submitInfo.signalSemaphores.size()),
            .pSignalSemaphores    = vkSignalSemaphores.data(),
        };

        if(submitInfo.waitStages.empty())
        {
            if(vkWaitStages.empty())
            {
                vkWaitStages.resize(submitInfo.waitSemaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            }
            info.pWaitDstStageMask = vkWaitStages.data();
        }
        vkSubmits.push_back(info);
    }

    VkResult result =
        vkQueueSubmit(getHandle(), vkSubmits.size(), vkSubmits.data(), pFence ? pFence->getHandle() : VK_NULL_HANDLE);
    return utils::getResult(result);
}

Result Queue::submit(const std::vector<QueueSubmitInfo2>& submitInfos)
{
    std::vector<QueueSubmitInfo2> si = submitInfos;
    std::vector<VkSubmitInfo2>    vkSubmitInfos;
    vkSubmitInfos.reserve(submitInfos.size());
    for(auto& submitInfo : si)
    {
        for(auto& cmd : submitInfo.commands)
        {
            cmd.sType      = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            cmd.pNext      = nullptr;
            cmd.deviceMask = 0;
        }
        for(auto& sig : submitInfo.signals)
        {
            sig.pNext = nullptr;
            sig.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            // TODO
            sig.stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            sig.deviceIndex = 0;
        }
        for(auto& wait : submitInfo.waits)
        {
            wait.pNext = nullptr;
            wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            // TODO
            wait.stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            wait.deviceIndex = 0;
        }
        vkSubmitInfos.push_back({
            .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext                    = nullptr,
            .waitSemaphoreInfoCount   = static_cast<uint32_t>(submitInfo.waits.size()),
            .pWaitSemaphoreInfos      = submitInfo.waits.data(),
            .commandBufferInfoCount   = static_cast<uint32_t>(submitInfo.commands.size()),
            .pCommandBufferInfos      = submitInfo.commands.data(),
            .signalSemaphoreInfoCount = static_cast<uint32_t>(submitInfo.signals.size()),
            .pSignalSemaphoreInfos    = submitInfo.signals.data(),
        });
    }
    return utils::getResult(vkQueueSubmit2(getHandle(), vkSubmitInfos.size(), vkSubmitInfos.data(), VK_NULL_HANDLE));
}
}  // namespace aph::vk
