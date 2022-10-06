#include "commandBuffer.h"
#include "buffer.h"
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
    vkCmdEndRenderPass(_handle);
}
void VulkanCommandBuffer::cmdSetViewport(VkViewport *viewport) {
    vkCmdSetViewport(_handle, 0, 1, viewport);
}
void VulkanCommandBuffer::cmdSetSissor(VkRect2D *scissor) {
    vkCmdSetScissor(_handle, 0, 1, scissor);
}
void VulkanCommandBuffer::cmdBindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    vkCmdBindPipeline(_handle, bindPoint, pipeline);
}
void VulkanCommandBuffer::cmdBindDescriptorSet(VkPipelineBindPoint    bindPoint,
                                               VkPipelineLayout       layout,
                                               uint32_t               firstSet,
                                               uint32_t               descriptorSetCount,
                                               const VkDescriptorSet *pDescriptorSets) {
    vkCmdBindDescriptorSets(_handle, bindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, 0, nullptr);
}
void VulkanCommandBuffer::cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VulkanBuffer *pBuffer, const VkDeviceSize *pOffsets) {
    vkCmdBindVertexBuffers(_handle, firstBinding, bindingCount, &pBuffer->getHandle(), pOffsets);
}
void VulkanCommandBuffer::cmdBindIndexBuffers(const VulkanBuffer *pBuffer, VkDeviceSize offset, VkIndexType indexType) {
    vkCmdBindIndexBuffer(_handle, pBuffer->getHandle(), offset, indexType);
}
void VulkanCommandBuffer::cmdPushConstants(VkPipelineLayout layout, VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void *pValues) {
    vkCmdPushConstants(_handle, layout, stage, offset, size, pValues);
}
void VulkanCommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) {
    vkCmdDrawIndexed(_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
} // namespace vkl
