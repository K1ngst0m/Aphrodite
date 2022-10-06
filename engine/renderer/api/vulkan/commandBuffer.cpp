#include "commandBuffer.h"
#include "commandPool.h"

namespace vkl {

VulkanCommandBuffer::~VulkanCommandBuffer() {
    m_pool->freeCommandBuffers(1, &_handle);
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool *pool, VkCommandBuffer handle)
    : m_pool(pool) {
    _handle = handle;
}

VkResult VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    if (m_isRecording) {
        return VK_NOT_READY;
    }

    // Begin command recording.
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = static_cast<VkCommandBufferUsageFlags>(flags);
    auto result                        = vkBeginCommandBuffer(_handle, &beginInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Mark CommandBuffer as recording and reset internal state.
    m_isRecording = true;

    return VK_SUCCESS;
}
VkResult VulkanCommandBuffer::end() {
    if (!m_isRecording) {
        return VK_NOT_READY;
    }

    return vkEndCommandBuffer(_handle);
}
VkResult VulkanCommandBuffer::reset() {
    if (_handle != VK_NULL_HANDLE)
        return vkResetCommandBuffer(_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    return VK_SUCCESS;
}

void VulkanCommandBuffer::cmdBeginRenderPass(const VkRenderPassBeginInfo *pBeginInfo) {
    vkCmdBeginRenderPass(_handle, pBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}
void VulkanCommandBuffer::cmdNextSubpass() {

}
void VulkanCommandBuffer::cmdEndRenderPass() {
}
} // namespace vkl
