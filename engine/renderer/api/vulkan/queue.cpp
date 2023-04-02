#include "queue.h"
#include "device.h"

namespace vkl {

VulkanQueue::VulkanQueue(VulkanDevice *device,
                         VkQueue queue, uint32_t queueFamilyIndex, uint32_t index,
                         const VkQueueFamilyProperties &propertiesd)
    : m_device(device), m_queueFamilyIndex(queueFamilyIndex), m_index(index), m_properties(propertiesd) {
    getHandle() = queue;
}

VkResult VulkanQueue::submit(uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    VkResult result = vkQueueSubmit(getHandle(), submitCount, pSubmits, fence);
    return result;
}

VkResult VulkanQueue::present(const VkPresentInfoKHR &presentInfo) {
    assert(getFlags() & VK_QUEUE_GRAPHICS_BIT);
    VkResult result = vkQueuePresentKHR(getHandle(), &presentInfo);
    return result;
}

} // namespace vkl
