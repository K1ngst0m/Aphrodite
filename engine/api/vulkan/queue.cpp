#include "queue.h"
#include "device.h"
#include <mutex>

namespace aph::vk
{

Queue::Queue(Device* pDevice, HandleType handle, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& propertiesd) :
    ResourceHandle(handle),
    m_queueFamilyIndex(queueFamilyIndex),
    m_index(index),
    m_properties(propertiesd),
    m_pDevice(pDevice)
{
    if(m_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        m_type = QueueType::Graphics;
    }
    else if(m_properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        m_type = QueueType::Compute;
    }
    else if(m_properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        m_type = QueueType::Transfer;
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

    std::lock_guard<std::mutex> lock{m_lock};
    VkResult result = m_pDevice->getDeviceTable()->vkQueueSubmit(getHandle(), vkSubmits.size(), vkSubmits.data(), pFence ? pFence->getHandle() : VK_NULL_HANDLE);
    return utils::getResult(result);
}

Result Queue::waitIdle()
{
    std::lock_guard<std::mutex> holder{m_lock};
    return utils::getResult(m_pDevice->getDeviceTable()->vkQueueWaitIdle(getHandle()));
}

Result Queue::present(const VkPresentInfoKHR& presentInfo)
{
    std::lock_guard<std::mutex> lock{m_lock};
    VkResult result = m_pDevice->getDeviceTable()->vkQueuePresentKHR(getHandle(), &presentInfo);
    return utils::getResult(result);
}
}  // namespace aph::vk
