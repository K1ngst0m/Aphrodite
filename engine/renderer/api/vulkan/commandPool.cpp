#include "commandPool.h"
#include "commandBuffer.h"

namespace vkl {

VulkanCommandPool *VulkanCommandPool::Create(VulkanDevice *device, uint32_t queueFamilyIndex, VkCommandPool pool) {
    VulkanCommandPool *instance = new VulkanCommandPool;
    instance->_device           = device;
    instance->_queueFamilyIndex = queueFamilyIndex;
    instance->getHandle()           = pool;
    return instance;
}
VkResult VulkanCommandPool::allocateCommandBuffers(uint32_t         commandBufferCount,
                                                   VkCommandBuffer *pCommandBuffers) {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();

    // Allocate a new command buffer.
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext                       = nullptr;
    allocInfo.commandPool                 = getHandle();
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = commandBufferCount;
    auto result                           = vkAllocateCommandBuffers(_device->getHandle(), &allocInfo, pCommandBuffers);

    // Unlock access to internal resources.
    _spinLock.Unlock();

    // Return result.
    return result;
}
void VulkanCommandPool::freeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    // Safe guard access to internal resources across threads.
    _spinLock.Lock();
    vkFreeCommandBuffers(_device->getHandle(), getHandle(), commandBufferCount, pCommandBuffers);
    _spinLock.Unlock();
}
uint32_t VulkanCommandPool::getQueueFamilyIndex() const {
    return _queueFamilyIndex;
}
} // namespace vkl
