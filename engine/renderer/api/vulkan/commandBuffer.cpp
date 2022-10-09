#include "commandBuffer.h"
#include "buffer.h"
#include "commandPool.h"
#include "framebuffer.h"
#include "image.h"
#include "renderpass.h"
#include "vkInit.hpp"

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

void VulkanCommandBuffer::cmdBeginRenderPass(const RenderPassBeginInfo *pBeginInfo) {
    VkRenderPassBeginInfo renderPassBeginInfo = vkl::init::renderPassBeginInfo();
    renderPassBeginInfo.renderPass            = pBeginInfo->pRenderPass->getHandle();
    renderPassBeginInfo.renderArea            = pBeginInfo->renderArea;
    renderPassBeginInfo.clearValueCount       = pBeginInfo->clearValueCount;
    renderPassBeginInfo.pClearValues          = pBeginInfo->pClearValues;
    renderPassBeginInfo.framebuffer           = pBeginInfo->pFramebuffer->getHandle(pBeginInfo->pRenderPass);

    vkCmdBeginRenderPass(_handle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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
void VulkanCommandBuffer::cmdCopyBuffer(VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size) {
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(_handle, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);
}
void VulkanCommandBuffer::cmdTransitionImageLayout(VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {

    VkImageMemoryBarrier imageMemoryBarrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->getHandle(),
    };

    const auto &imageCreateInfo         = image->getCreateInfo();
    imageMemoryBarrier.subresourceRange = {
        .aspectMask     = vkl::utils::getImageAspectFlags(static_cast<VkFormat>(imageCreateInfo.format)),
        .baseMipLevel   = 0,
        .levelCount     = imageCreateInfo.mipLevels,
        .baseArrayLayer = 0,
        .layerCount     = imageCreateInfo.arrayLayers,
    };

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0) {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    vkCmdPipelineBarrier(_handle,
                         srcStageMask,
                         dstStageMask,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier);
}
void VulkanCommandBuffer::cmdCopyBufferToImage(VulkanBuffer *buffer, VulkanImage *image) {
    VkBufferImageCopy region{
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
    };

    region.imageSubresource = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image->getWidth(), image->getHeight(), 1};

    vkCmdCopyBufferToImage(_handle, buffer->getHandle(), image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
void VulkanCommandBuffer::cmdCopyImage(VulkanImage *srcImage, VulkanImage *dstImage) {
    // Copy region for transfer from framebuffer to cube face
    VkImageCopy copyRegion = {};
    copyRegion.srcOffset   = {0, 0, 0};
    copyRegion.dstOffset   = {0, 0, 0};

    VkImageSubresourceLayers subresourceLayers{
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    copyRegion.srcSubresource = subresourceLayers;
    copyRegion.dstSubresource = subresourceLayers;
    copyRegion.extent.width   = srcImage->getWidth();
    copyRegion.extent.height  = srcImage->getHeight();
    copyRegion.extent.depth   = 1;

    vkCmdCopyImage(_handle,
                   srcImage->getHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage->getHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &copyRegion);
}
void VulkanCommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(_handle, vertexCount, instanceCount, firstVertex, firstInstance);
}
} // namespace vkl
