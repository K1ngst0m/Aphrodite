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
        vkSubmits.push_back(info);
    }

    VkResult result = vkQueueSubmit(getHandle(), vkSubmits.size(), vkSubmits.data(), fence);
    return result;
}

}  // namespace aph::vk
