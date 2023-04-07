#include "queue.h"
#include "device.h"

namespace aph {

VulkanQueue::VulkanQueue(VkQueue queue, uint32_t queueFamilyIndex, uint32_t index,
                         const VkQueueFamilyProperties &propertiesd)
    : m_queueFamilyIndex(queueFamilyIndex), m_index(index), m_properties(propertiesd) {
    getHandle() = queue;
}

VkResult VulkanQueue::submit(const std::vector<QueueSubmitInfo>& submitInfos, VkFence fence) {
    std::vector<VkSubmitInfo> finalSubmits;
    std::vector<VkCommandBuffer> cmds;

    for (const auto &submitInfo : submitInfos)
    {
        cmds.reserve(submitInfo.commandBuffers.size());
        for (auto *cmd : submitInfo.commandBuffers){
            cmds.push_back(cmd->getHandle());
        }
        VkSubmitInfo info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = static_cast<uint32_t>(submitInfo.waitSemaphores.size()),
            .pWaitSemaphores = submitInfo.waitSemaphores.data(),
            .pWaitDstStageMask = submitInfo.waitStages.data(),
            .commandBufferCount = static_cast<uint32_t>(cmds.size()),
            .pCommandBuffers = cmds.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(submitInfo.signalSemaphores.size()),
            .pSignalSemaphores = submitInfo.signalSemaphores.data(),
        };
        finalSubmits.push_back(info);
    }

    VkResult result = vkQueueSubmit(getHandle(), finalSubmits.size(), finalSubmits.data(), fence);
    return result;
}

} // namespace aph
