#include "commandPool.h"

namespace vkl {

VkResult VulkanCommandPool::create(VulkanDevice *device, uint32_t queueFamilyIndex, VulkanCommandPool **ppCommandPool) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = queueFamilyIndex;
    poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool handle = VK_NULL_HANDLE;
    auto          result = vkCreateCommandPool(device->getLogicalDevice(), &poolInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    *ppCommandPool                      = new VulkanCommandPool;
    (*ppCommandPool)->_device           = device;
    (*ppCommandPool)->_queueFamilyIndex = queueFamilyIndex;
    (*ppCommandPool)->_handle           = handle;
    return VK_SUCCESS;
}
VkResult VulkanCommandPool::allocateCommandBuffers(const void      *pNext,
                                                   uint32_t         commandBufferCount,
                                                   VkCommandBuffer *pCommandBuffers) {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();

    // Allocate a new command buffer.
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext                       = pNext;
    allocInfo.commandPool                 = _handle;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = commandBufferCount;
    auto result                           = vkAllocateCommandBuffers(_device->getLogicalDevice(), &allocInfo, pCommandBuffers);

    // Unlock access to internal resources.
    _spinLock.Unlock();

    // Return result.
    return result;
}
void VulkanCommandPool::freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();
    vkFreeCommandBuffers(_device->getLogicalDevice(), _handle, commandBufferCount, pCommandBuffers);
    _spinLock.Unlock();
}
uint32_t VulkanCommandPool::getQueueFamilyIndex() const {
    return _queueFamilyIndex;
}
} // namespace vkl
