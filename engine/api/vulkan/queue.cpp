#include "queue.h"
#include "device.h"

namespace aph::vk
{

Queue::Queue(VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& propertiesd) :
    m_queueFamilyIndex(queueFamilyIndex),
    m_index(index),
    m_properties(propertiesd)
{
    getHandle() = queue;
}

VkResult Queue::submit(const std::vector<QueueSubmitInfo>& submitInfos, VkFence fence)
{
    std::vector<VkSubmitInfo>    vkSubmits;
    std::vector<VkCommandBuffer> vkCmds;
    std::vector<VkPipelineStageFlags> vkWaitStages;

    for(const auto& submitInfo : submitInfos)
    {
        uint32_t cmdOffset = {static_cast<uint32_t>(vkCmds.size())};
        uint32_t cmdSize   = {static_cast<uint32_t>(submitInfo.commandBuffers.size())};

        for(auto* cmd : submitInfo.commandBuffers)
        {
            vkCmds.push_back(cmd->getHandle());
        }

        VkSubmitInfo info{
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = static_cast<uint32_t>(submitInfo.waitSemaphores.size()),
            .pWaitSemaphores      = submitInfo.waitSemaphores.data(),
            .pWaitDstStageMask    = submitInfo.waitStages.data(),
            .commandBufferCount   = cmdSize,
            .pCommandBuffers      = &vkCmds[cmdOffset],
            .signalSemaphoreCount = static_cast<uint32_t>(submitInfo.signalSemaphores.size()),
            .pSignalSemaphores    = submitInfo.signalSemaphores.data(),
        };

        if (submitInfo.waitStages.empty())
        {
            if (vkWaitStages.empty())
            {
                vkWaitStages.resize(submitInfo.waitSemaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            }
            info.pWaitDstStageMask = vkWaitStages.data();
        }
        vkSubmits.push_back(info);
    }

    VkResult result = vkQueueSubmit(getHandle(), vkSubmits.size(), vkSubmits.data(), fence);
    return result;
}

VkResult Queue::submit(const std::vector<QueueSubmitInfo2>& submitInfos)
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
    return vkQueueSubmit2(getHandle(), vkSubmitInfos.size(), vkSubmitInfos.data(), VK_NULL_HANDLE);
}
}  // namespace aph::vk
