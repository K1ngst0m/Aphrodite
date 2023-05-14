#include "commandBuffer.h"
#include "api/gpuResource.h"
#include "device.h"

namespace aph::vk
{

CommandBuffer::~CommandBuffer()
{
    m_pool->freeCommandBuffers(1, &m_handle);
}

CommandBuffer::CommandBuffer(Device* pDevice, CommandPool* pool, VkCommandBuffer handle, uint32_t queueFamilyIndices) :
    m_pDevice(pDevice),
    m_pDeviceTable(pDevice->getDeviceTable()),
    m_pool(pool),
    m_state(CommandBufferState::INITIAL),
    m_queueFamilyType(queueFamilyIndices)
{
    getHandle() = handle;
}

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    if(m_state == CommandBufferState::RECORDING)
    {
        return VK_NOT_READY;
    }

    // Begin command recording.
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = static_cast<VkCommandBufferUsageFlags>(flags),
    };
    auto result = m_pDeviceTable->vkBeginCommandBuffer(m_handle, &beginInfo);
    if(result != VK_SUCCESS)
    {
        return result;
    }

    // Mark CommandBuffer as recording and reset internal state.
    m_graphicsState = CommandGraphicsState();
    m_state         = CommandBufferState::RECORDING;

    return VK_SUCCESS;
}

VkResult CommandBuffer::end()
{
    if(m_state != CommandBufferState::RECORDING)
    {
        return VK_NOT_READY;
    }

    m_state = CommandBufferState::EXECUTABLE;

    return m_pDeviceTable->vkEndCommandBuffer(m_handle);
}

VkResult CommandBuffer::reset()
{
    if(m_handle != VK_NULL_HANDLE)
        return m_pDeviceTable->vkResetCommandBuffer(m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    m_state = CommandBufferState::INITIAL;
    return VK_SUCCESS;
}

void CommandBuffer::setViewport(const VkViewport& viewport)
{
    m_pDeviceTable->vkCmdSetViewport(m_handle, 0, 1, &viewport);
    m_graphicsState.viewport = viewport;
}
void CommandBuffer::setSissor(const VkRect2D& scissor)
{
    m_pDeviceTable->vkCmdSetScissor(m_handle, 0, 1, &scissor);
    m_graphicsState.scissor = scissor;
}
void CommandBuffer::bindPipeline(Pipeline* pPipeline)
{
    m_graphicsState.pPipeline = pPipeline;
    m_pDeviceTable->vkCmdBindPipeline(m_handle, pPipeline->getBindPoint(), pPipeline->getHandle());
}
void CommandBuffer::bindDescriptorSet(uint32_t firstSet, uint32_t descriptorSetCount,
                                      const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                      const uint32_t* pDynamicOffset)
{
    APH_ASSERT(m_graphicsState.pPipeline != nullptr);
    m_pDeviceTable->vkCmdBindDescriptorSets(m_handle, m_graphicsState.pPipeline->getBindPoint(),
                                            m_graphicsState.pPipeline->getPipelineLayout(), firstSet,
                                            descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffset);
}
void CommandBuffer::bindVertexBuffers(const Buffer* pBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                      const std::vector<VkDeviceSize>& offsets)
{
    m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, firstBinding, bindingCount, &pBuffer->getHandle(), offsets.data());
}
void CommandBuffer::bindIndexBuffers(const Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
{
    m_pDeviceTable->vkCmdBindIndexBuffer(m_handle, pBuffer->getHandle(), offset, indexType);
}
void CommandBuffer::pushConstants(uint32_t offset, uint32_t size, const void* pValues)
{
    APH_ASSERT(m_graphicsState.pPipeline != nullptr);
    auto stage = m_graphicsState.pPipeline->getConstantShaderStage(offset, size);
    m_pDeviceTable->vkCmdPushConstants(m_handle, m_graphicsState.pPipeline->getPipelineLayout(), stage, offset, size,
                                       pValues);
}
void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                                uint32_t firstInstance)
{
    m_pDeviceTable->vkCmdDrawIndexed(m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void CommandBuffer::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    m_pDeviceTable->vkCmdCopyBuffer(m_handle, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);
}
void CommandBuffer::transitionImageLayout(Image* image, VkImageLayout newLayout,
                                          VkImageSubresourceRange* pSubResourceRange, VkPipelineStageFlags srcStageMask,
                                          VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier2 imageMemoryBarrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .oldLayout           = image->m_layout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->getHandle(),
    };

    const auto& imageCreateInfo = image->getCreateInfo();
    if(pSubResourceRange)
    {
        imageMemoryBarrier.subresourceRange = *pSubResourceRange;
    }
    else
    {
        imageMemoryBarrier.subresourceRange = {
            .aspectMask     = utils::getImageAspect(imageCreateInfo.format),
            .baseMipLevel   = 0,
            .levelCount     = imageCreateInfo.mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = imageCreateInfo.arrayLayers,
        };
    }

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch(image->m_layout)
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
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
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
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask =
            imageMemoryBarrier.dstAccessMask | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if(imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // TODO debug
    {
        imageMemoryBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT |
                                           VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageMemoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT |
                                           VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }

    VkDependencyInfoKHR dependencyInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                   = nullptr,
        .dependencyFlags         = 0,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &imageMemoryBarrier,
    };

    m_pDeviceTable->vkCmdPipelineBarrier2(m_handle, &dependencyInfo);
    image->m_layout = newLayout;
}
void CommandBuffer::copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions)
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
void CommandBuffer::copyImage(Image* srcImage, Image* dstImage)
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
void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    m_pDeviceTable->vkCmdDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::imageMemoryBarrier(Image* image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                       VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier imageMemoryBarrier = init::imageMemoryBarrier();
    imageMemoryBarrier.srcAccessMask        = srcAccessMask;
    imageMemoryBarrier.dstAccessMask        = dstAccessMask;
    imageMemoryBarrier.oldLayout            = image->m_layout;
    imageMemoryBarrier.newLayout            = newImageLayout;
    imageMemoryBarrier.image                = image->getHandle();
    imageMemoryBarrier.subresourceRange     = subresourceRange;

    m_pDeviceTable->vkCmdPipelineBarrier(m_handle, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                                         &imageMemoryBarrier);
    image->m_layout = newImageLayout;
}
void CommandBuffer::blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions,
                              VkFilter filter)
{
    m_pDeviceTable->vkCmdBlitImage(m_handle, srcImage->getHandle(), srcImageLayout, dstImage->getHandle(),
                                   dstImageLayout, 1, pRegions, filter);
}
uint32_t CommandBuffer::getQueueFamilyIndices() const
{
    return m_queueFamilyType;
};
void CommandBuffer::beginRendering(const VkRenderingInfo& renderingInfo)
{
    // uint32_t colorCount = renderingInfo.colorAttachmentCount;
    // for (uint32_t idx = 0; idx < colorCount; idx++)
    // {
    //     m_graphicsState.colorAttachments.resize(colorCount);
    // }
    // memcpy(m_graphicsState.colorAttachments.data(), renderingInfo.pColorAttachments,
    // sizeof(VkRenderingAttachmentInfo) * colorCount); memcpy(&m_graphicsState.depthAttachment,
    // renderingInfo.pDepthAttachment, sizeof(VkRenderingAttachmentInfo));
    m_pDeviceTable->vkCmdBeginRendering(getHandle(), &renderingInfo);
    // for (auto colorAttachment : m_graphicsState.colorAttachments){
    // transitionImageLayout(colorAttachment.imageView, , VkImageLayout newLayout)
    // }
}
void CommandBuffer::endRendering()
{
    m_pDeviceTable->vkCmdEndRendering(getHandle());
}
void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_pDeviceTable->vkCmdDispatch(getHandle(), groupCountX, groupCountY, groupCountZ);
}
void CommandBuffer::dispatch(Buffer* pBuffer, VkDeviceSize offset)
{
    vkCmdDispatchIndirect(getHandle(), pBuffer->getHandle(), offset);
}
void CommandBuffer::draw(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    vkCmdDrawIndirect(getHandle(), pBuffer->getHandle(), offset, drawCount, stride);
}
void CommandBuffer::bindDescriptorSet(const std::vector<VkDescriptorSet>& pDescriptorSets, uint32_t firstSet)
{
    APH_ASSERT(m_graphicsState.pPipeline != nullptr);
    m_pDeviceTable->vkCmdBindDescriptorSets(m_handle, m_graphicsState.pPipeline->getBindPoint(),
                                            m_graphicsState.pPipeline->getPipelineLayout(), firstSet,
                                            pDescriptorSets.size(), pDescriptorSets.data(), 0, nullptr);
}
void CommandBuffer::setRenderTarget(const std::vector<AttachmentInfo>& colors, const AttachmentInfo& depth)
{
    m_graphicsState.colorAttachments = colors;
    m_graphicsState.depthAttachment  = depth;
}
void CommandBuffer::beginRendering(VkRect2D renderArea)
{
    APH_ASSERT(!m_graphicsState.colorAttachments.empty() || m_graphicsState.depthAttachment.has_value());
    std::vector<VkRenderingAttachmentInfo> vkColors;
    VkRenderingAttachmentInfo              vkDepth;
    vkColors.reserve(m_graphicsState.colorAttachments.size());
    for(const auto& color : m_graphicsState.colorAttachments)
    {
        auto&                     image = color.image;
        VkRenderingAttachmentInfo vkColor{.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                          .imageView   = image->getView()->getHandle(),
                                          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                          .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                                          .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}}};
        if(color.layout.has_value())
        {
            vkColor.imageLayout = color.layout.value();
        }
        if(color.clear.has_value())
        {
            vkColor.clearValue = color.clear.value();
        }
        if(color.loadOp.has_value())
        {
            vkColor.loadOp = color.loadOp.value();
        }
        if(color.storeOp.has_value())
        {
            vkColor.storeOp = color.storeOp.value();
        }
        vkColors.push_back(vkColor);
        transitionImageLayout(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    VkRenderingInfo renderingInfo{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = renderArea,
        .layerCount           = 1,
        .colorAttachmentCount = static_cast<uint32_t>(vkColors.size()),
        .pColorAttachments    = vkColors.data(),
        .pDepthAttachment     = nullptr,
    };

    if(m_graphicsState.depthAttachment.has_value())
    {
        auto& image = m_graphicsState.depthAttachment.value().image;
        vkDepth     = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = image->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue  = {.depthStencil{1.0f, 0}},
        };
        if(m_graphicsState.depthAttachment.value().layout.has_value())
        {
            vkDepth.imageLayout = m_graphicsState.depthAttachment.value().layout.value();
        }
        if(m_graphicsState.depthAttachment.value().storeOp.has_value())
        {
            vkDepth.storeOp = m_graphicsState.depthAttachment.value().storeOp.value();
        }
        if(m_graphicsState.depthAttachment.value().loadOp.has_value())
        {
            vkDepth.loadOp = m_graphicsState.depthAttachment.value().loadOp.value();
        }
        if(m_graphicsState.depthAttachment.value().clear.has_value())
        {
            vkDepth.clearValue = m_graphicsState.depthAttachment.value().clear.value();
        }
        transitionImageLayout(image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        renderingInfo.pDepthAttachment = &vkDepth;
    }

    m_pDeviceTable->vkCmdBeginRendering(getHandle(), &renderingInfo);
}
void CommandBuffer::setRenderTarget(const std::vector<Image*>& colors, Image* depth)
{
    for(auto color : colors)
    {
        m_graphicsState.colorAttachments.push_back({.image = color});
    }
    m_graphicsState.depthAttachment = {.image = depth};
}
void CommandBuffer::bindDescriptorSet(uint32_t setIdx)
{
}
}  // namespace aph::vk
