#include "commandBuffer.h"
#include "api/gpuResource.h"
#include "device.h"

namespace aph
{

VulkanCommandBuffer::~VulkanCommandBuffer() { m_pool->freeCommandBuffers(1, &m_handle); }

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* pDevice, VulkanCommandPool* pool, VkCommandBuffer handle,
                                         uint32_t queueFamilyIndices) :
    m_pDevice(pDevice),
    m_pDeviceTable(pDevice->getDeviceTable()),
    m_pool(pool),
    m_state(CommandBufferState::INITIAL),
    m_queueFamilyType(queueFamilyIndices)
{
    getHandle() = handle;
}

VkResult VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    if(m_state == CommandBufferState::RECORDING) { return VK_NOT_READY; }

    // Begin command recording.
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = static_cast<VkCommandBufferUsageFlags>(flags),
    };
    auto result = m_pDeviceTable->vkBeginCommandBuffer(m_handle, &beginInfo);
    if(result != VK_SUCCESS) { return result; }

    // Mark CommandBuffer as recording and reset internal state.
    m_state = CommandBufferState::RECORDING;

    return VK_SUCCESS;
}

VkResult VulkanCommandBuffer::end()
{
    if(m_state != CommandBufferState::RECORDING) { return VK_NOT_READY; }

    m_state = CommandBufferState::EXECUTABLE;

    return m_pDeviceTable->vkEndCommandBuffer(m_handle);
}

VkResult VulkanCommandBuffer::reset()
{
    if(m_handle != VK_NULL_HANDLE)
        return m_pDeviceTable->vkResetCommandBuffer(m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    m_state = CommandBufferState::INITIAL;
    return VK_SUCCESS;
}

void VulkanCommandBuffer::setViewport(const VkViewport& viewport)
{
    m_pDeviceTable->vkCmdSetViewport(m_handle, 0, 1, &viewport);
}
void VulkanCommandBuffer::setSissor(const VkRect2D& scissor)
{
    m_pDeviceTable->vkCmdSetScissor(m_handle, 0, 1, &scissor);
}
void VulkanCommandBuffer::bindPipeline(VulkanPipeline* pPipeline)
{
    m_pDeviceTable->vkCmdBindPipeline(m_handle, pPipeline->getBindPoint(), pPipeline->getHandle());
}
void VulkanCommandBuffer::bindDescriptorSet(VulkanPipeline* pPipeline, uint32_t firstSet, uint32_t descriptorSetCount,
                                            const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                            const uint32_t* pDynamicOffset)
{
    m_pDeviceTable->vkCmdBindDescriptorSets(m_handle, pPipeline->getBindPoint(), pPipeline->getPipelineLayout(),
                                            firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount,
                                            pDynamicOffset);
}
void VulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VulkanBuffer* pBuffer,
                                            const std::vector<VkDeviceSize>& offsets)
{
    m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, firstBinding, bindingCount, &pBuffer->getHandle(), offsets.data());
}
void VulkanCommandBuffer::bindIndexBuffers(const VulkanBuffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
{
    m_pDeviceTable->vkCmdBindIndexBuffer(m_handle, pBuffer->getHandle(), offset, indexType);
}
void VulkanCommandBuffer::pushConstants(VulkanPipeline* pPipeline, const std::vector<ShaderStage>& stages,
                                        uint32_t offset, uint32_t size, const void* pValues)
{
    m_pDeviceTable->vkCmdPushConstants(m_handle, pPipeline->getPipelineLayout(), utils::VkCast(stages), offset, size,
                                       pValues);
}
void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                      uint32_t vertexOffset, uint32_t firstInstance)
{
    m_pDeviceTable->vkCmdDrawIndexed(m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void VulkanCommandBuffer::copyBuffer(VulkanBuffer* srcBuffer, VulkanBuffer* dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    m_pDeviceTable->vkCmdCopyBuffer(m_handle, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);
}
void VulkanCommandBuffer::transitionImageLayout(VulkanImage* image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                VkImageSubresourceRange* pSubResourceRange,
                                                VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier imageMemoryBarrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->getHandle(),
    };

    const auto& imageCreateInfo = image->getCreateInfo();
    if(pSubResourceRange) { imageMemoryBarrier.subresourceRange = *pSubResourceRange; }
    else
    {
        imageMemoryBarrier.subresourceRange = {
            .aspectMask     = aph::utils::getImageAspectFlags(imageCreateInfo.format),
            .baseMipLevel   = 0,
            .levelCount     = imageCreateInfo.mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = imageCreateInfo.arrayLayers,
        };
    }

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch(oldLayout)
    {
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
    switch(newLayout)
    {
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
        imageMemoryBarrier.dstAccessMask =
            imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if(imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    m_pDeviceTable->vkCmdPipelineBarrier(m_handle, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                                         &imageMemoryBarrier);
}
void VulkanCommandBuffer::copyBufferToImage(VulkanBuffer* buffer, VulkanImage* image,
                                            const std::vector<VkBufferImageCopy>& regions)
{
    if(regions.empty())
    {
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
        m_pDeviceTable->vkCmdCopyBufferToImage(m_handle, buffer->getHandle(), image->getHandle(),
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    else
    {
        m_pDeviceTable->vkCmdCopyBufferToImage(m_handle, buffer->getHandle(), image->getHandle(),
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
    }
}
void VulkanCommandBuffer::copyImage(VulkanImage* srcImage, VulkanImage* dstImage)
{
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

    m_pDeviceTable->vkCmdCopyImage(m_handle, srcImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   dstImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}
void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                               uint32_t firstInstance)
{
    m_pDeviceTable->vkCmdDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::imageMemoryBarrier(VulkanImage* image, VkAccessFlags srcAccessMask,
                                             VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout,
                                             VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask,
                                             VkPipelineStageFlags    dstStageMask,
                                             VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier imageMemoryBarrier = aph::init::imageMemoryBarrier();
    imageMemoryBarrier.srcAccessMask        = srcAccessMask;
    imageMemoryBarrier.dstAccessMask        = dstAccessMask;
    imageMemoryBarrier.oldLayout            = oldImageLayout;
    imageMemoryBarrier.newLayout            = newImageLayout;
    imageMemoryBarrier.image                = image->getHandle();
    imageMemoryBarrier.subresourceRange     = subresourceRange;

    m_pDeviceTable->vkCmdPipelineBarrier(m_handle, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                                         &imageMemoryBarrier);
}
void VulkanCommandBuffer::blitImage(VulkanImage* srcImage, VkImageLayout srcImageLayout, VulkanImage* dstImage,
                                    VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions,
                                    VkFilter filter)
{
    m_pDeviceTable->vkCmdBlitImage(m_handle, srcImage->getHandle(), srcImageLayout, dstImage->getHandle(),
                                   dstImageLayout, 1, pRegions, filter);
}
uint32_t VulkanCommandBuffer::getQueueFamilyIndices() const { return m_queueFamilyType; };
void     VulkanCommandBuffer::beginRendering(const VkRenderingInfo& renderingInfo)
{
    m_pDeviceTable->vkCmdBeginRendering(getHandle(), &renderingInfo);
}
void VulkanCommandBuffer::endRendering() { m_pDeviceTable->vkCmdEndRendering(getHandle()); }
void VulkanCommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_pDeviceTable->vkCmdDispatch(getHandle(), groupCountX, groupCountY, groupCountZ);
}
void VulkanCommandBuffer::pushDescriptorSet(VulkanPipeline* pipeline, const std::vector<VkWriteDescriptorSet>& writes,
                                            uint32_t setIdx)
{
    m_pDeviceTable->vkCmdPushDescriptorSetKHR(getHandle(), pipeline->getBindPoint(), pipeline->getPipelineLayout(),
                                              setIdx, writes.size(), writes.data());
}
}  // namespace aph
