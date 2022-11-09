#include "queue.h"
#include "commandBuffer.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"

namespace vkl {

VulkanQueue::VulkanQueue(VulkanDevice *device,
                         VkQueue queue, uint32_t queueFamilyIndex, uint32_t index,
                         const VkQueueFamilyProperties &propertiesd)
    : m_device(device), m_queueFamilyIndex(queueFamilyIndex), m_index(index), m_properties(propertiesd) {
    _handle = queue;
}

uint32_t VulkanQueue::getFamilyIndex() const {
    return m_queueFamilyIndex;
}

uint32_t VulkanQueue::getIndex() const {
    return m_index;
}

VkQueueFlags VulkanQueue::getFlags() const {
    return m_properties.queueFlags;
}

VkResult VulkanQueue::waitIdle() {
    return VkResult(vkQueueWaitIdle(_handle));
}

VkResult VulkanQueue::submit(uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    VkResult result = vkQueueSubmit(_handle, submitCount, pSubmits, fence);
    return result;
}

VkResult VulkanQueue::present(const PresentInfo *pPresentInfo) {
    return VK_NOT_READY;
}

} // namespace vkl
