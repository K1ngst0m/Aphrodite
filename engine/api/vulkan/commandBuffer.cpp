#include "commandBuffer.h"
#include "api/gpuResource.h"
#include "device.h"

namespace aph::vk
{

CommandBuffer::CommandBuffer(Device* pDevice, VkCommandPool pool, VkCommandBuffer handle, Queue* pQueue) :
    m_pDevice(pDevice),
    m_pDeviceTable(pDevice->getDeviceTable()),
    m_pool(pool),
    m_state(CommandBufferState::INITIAL),
    m_pQueue(pQueue)
{
    getHandle() = handle;
}

CommandBuffer::~CommandBuffer()
{
    m_pDeviceTable->vkFreeCommandBuffers(m_pDevice->getHandle(), m_pool, 1, &getHandle());
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
    m_commandState = {};
    m_state        = CommandBufferState::RECORDING;

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
    m_commandState.viewport = viewport;
}
void CommandBuffer::setScissor(const VkRect2D& scissor)
{
    m_commandState.scissor = scissor;
}
void CommandBuffer::bindPipeline(Pipeline* pPipeline)
{
    m_commandState.pPipeline = pPipeline;
}

void CommandBuffer::bindDescriptorSet(uint32_t firstSet, uint32_t descriptorSetCount,
                                      const DescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                      const uint32_t* pDynamicOffset)
{
    APH_ASSERT(m_commandState.pPipeline != nullptr);
    m_pDeviceTable->vkCmdBindDescriptorSets(
        m_handle, m_commandState.pPipeline->getBindPoint(), m_commandState.pPipeline->getProgram()->getPipelineLayout(),
        firstSet, descriptorSetCount, &pDescriptorSets->getHandle(), dynamicOffsetCount, pDynamicOffset);
}

void CommandBuffer::bindVertexBuffers(Buffer* pBuffer, uint32_t binding, uint32_t offset)
{
    APH_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS);

    VkBuffer vkBuffer                             = pBuffer->getHandle();
    m_commandState.vertexBinding.buffers[binding] = vkBuffer;
    m_commandState.vertexBinding.offsets[binding] = offset;
    m_commandState.vertexBinding.dirty |= 1u << binding;
}

void CommandBuffer::bindIndexBuffers(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
{
    m_commandState.index = {.buffer = pBuffer->getHandle(), .offset = offset, .indexType = indexType};
}

void CommandBuffer::pushConstants(uint32_t offset, uint32_t size, const void* pValues)
{
    APH_ASSERT(m_commandState.pPipeline != nullptr);
    auto stage = m_commandState.pPipeline->getProgram()->getConstantShaderStage(offset, size);
    m_pDeviceTable->vkCmdPushConstants(m_handle, m_commandState.pPipeline->getProgram()->getPipelineLayout(), stage,
                                       offset, size, pValues);
}
void CommandBuffer::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{.size = size};
    m_pDeviceTable->vkCmdCopyBuffer(m_handle, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);
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
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions,
                              VkFilter filter)
{
    m_pDeviceTable->vkCmdBlitImage(m_handle, srcImage->getHandle(), srcImageLayout, dstImage->getHandle(),
                                   dstImageLayout, 1, pRegions, filter);
}

void CommandBuffer::endRendering()
{
    m_pDeviceTable->vkCmdEndRendering(getHandle());
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    flushComputeCommand();
    m_pDeviceTable->vkCmdDispatch(getHandle(), groupCountX, groupCountY, groupCountZ);
}
void CommandBuffer::dispatch(Buffer* pBuffer, VkDeviceSize offset)
{
    flushComputeCommand();
    vkCmdDispatchIndirect(getHandle(), pBuffer->getHandle(), offset);
}
void CommandBuffer::draw(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    flushGraphicsCommand();
    vkCmdDrawIndirect(getHandle(), pBuffer->getHandle(), offset, drawCount, stride);
}
void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                                uint32_t firstInstance)
{
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDrawIndexed(m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void CommandBuffer::bindDescriptorSet(const std::vector<DescriptorSet*>& descriptorSets, uint32_t firstSet)
{
    APH_ASSERT(m_commandState.pPipeline != nullptr);
    std::vector<VkDescriptorSet> vkSets;
    vkSets.reserve(descriptorSets.size());
    for(auto set : descriptorSets)
    {
        vkSets.push_back(set->getHandle());
    }
    m_pDeviceTable->vkCmdBindDescriptorSets(m_handle, m_commandState.pPipeline->getBindPoint(),
                                            m_commandState.pPipeline->getProgram()->getPipelineLayout(), firstSet,
                                            vkSets.size(), vkSets.data(), 0, nullptr);
}

void CommandBuffer::beginRendering(VkRect2D renderArea, const std::vector<Image*>& colors, Image* depth)
{
    std::vector<AttachmentInfo> colorAttachments = {};
    AttachmentInfo              depthAttachment  = {};
    colorAttachments.reserve(colors.size());
    for(auto color : colors)
    {
        colorAttachments.push_back({.image = color});
    }

    depthAttachment = {.image = depth};
    beginRendering(renderArea, colorAttachments, depthAttachment);
}

void CommandBuffer::beginRendering(VkRect2D renderArea, const std::vector<AttachmentInfo>& colors,
                                   const AttachmentInfo& depth)
{
    m_commandState.colorAttachments = colors;
    m_commandState.depthAttachment  = depth;
    APH_ASSERT(!m_commandState.colorAttachments.empty() || m_commandState.depthAttachment.has_value());
    std::vector<VkRenderingAttachmentInfo> vkColors;
    VkRenderingAttachmentInfo              vkDepth;
    vkColors.reserve(m_commandState.colorAttachments.size());
    for(const auto& color : m_commandState.colorAttachments)
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

        aph::vk::ImageBarrier barrier{
            .pImage       = image,
            .currentState = image->getResourceState(),
            .newState     = aph::RESOURCE_STATE_RENDER_TARGET,
        };
        insertBarrier({barrier});
    }

    VkRenderingInfo renderingInfo{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = renderArea,
        .layerCount           = 1,
        .colorAttachmentCount = static_cast<uint32_t>(vkColors.size()),
        .pColorAttachments    = vkColors.data(),
        .pDepthAttachment     = nullptr,
    };

    if(m_commandState.depthAttachment.has_value() && m_commandState.depthAttachment->image != nullptr)
    {
        auto& image = m_commandState.depthAttachment->image;
        vkDepth     = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = image->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue  = {.depthStencil{1.0f, 0}},
        };
        if(m_commandState.depthAttachment->layout.has_value())
        {
            vkDepth.imageLayout = m_commandState.depthAttachment->layout.value();
        }
        if(m_commandState.depthAttachment->storeOp.has_value())
        {
            vkDepth.storeOp = m_commandState.depthAttachment->storeOp.value();
        }
        if(m_commandState.depthAttachment->loadOp.has_value())
        {
            vkDepth.loadOp = m_commandState.depthAttachment->loadOp.value();
        }
        if(m_commandState.depthAttachment->clear.has_value())
        {
            vkDepth.clearValue = m_commandState.depthAttachment->clear.value();
        }
        // debug layout
        // transitionImageLayout(image, VK_IMAGE_LAYOUT_GENERAL);
        aph::vk::ImageBarrier barrier{
            .pImage       = image,
            .currentState = image->getResourceState(),
            .newState     = aph::RESOURCE_STATE_DEPTH_STENCIL,
        };
        insertBarrier({barrier});

        renderingInfo.pDepthAttachment = &vkDepth;
    }
    m_pDeviceTable->vkCmdBeginRendering(getHandle(), &renderingInfo);
}

void CommandBuffer::flushComputeCommand()
{
    m_pDeviceTable->vkCmdBindPipeline(m_handle, m_commandState.pPipeline->getBindPoint(),
                                      m_commandState.pPipeline->getHandle());
}
void CommandBuffer::flushGraphicsCommand()
{
    m_pDeviceTable->vkCmdSetViewport(m_handle, 0, 1, &m_commandState.viewport);
    m_pDeviceTable->vkCmdSetScissor(m_handle, 0, 1, &m_commandState.scissor);

    aph::utils::forEachBitRange(m_commandState.vertexBinding.dirty, [&](uint32_t binding, uint32_t bindingCount) {
#ifdef APH_DEBUG
        for(unsigned i = binding; i < binding + bindingCount; i++)
            APH_ASSERT(m_commandState.vertexBinding.buffers[i] != VK_NULL_HANDLE);
#endif
        m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, binding, bindingCount,
                                               m_commandState.vertexBinding.buffers + binding,
                                               m_commandState.vertexBinding.offsets + binding);
    });

    m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, 0, 1, &m_commandState.vertexBinding.buffers[0],
                                           m_commandState.vertexBinding.offsets);
    m_pDeviceTable->vkCmdBindIndexBuffer(m_handle, m_commandState.index.buffer, m_commandState.index.offset,
                                         m_commandState.index.indexType);
    m_pDeviceTable->vkCmdBindPipeline(m_handle, m_commandState.pPipeline->getBindPoint(),
                                      m_commandState.pPipeline->getHandle());
}

void CommandBuffer::bindBuffer(uint32_t set, uint32_t binding, ResourceType type, Buffer* buffer, VkDeviceSize offset,
                               VkDeviceSize size)
{
    switch(type)
    {
    case ResourceType::UniformBuffer:
    case ResourceType::StorageBuffer:
    {
        auto& b                 = m_commandState.resourceBindings.bindings;
        b[set][binding]->buffer = {.buffer = buffer->getHandle(), .offset = offset, .range = size};
    }
    break;
    default:
        APH_ASSERT(false);
        VK_LOG_ERR("Invalid resources type binding.");
        break;
    }
    m_commandState.resourceBindings.bindings[set][binding]->resType = utils::VkCast(type);
}
void CommandBuffer::bindTexture(uint32_t set, uint32_t binding, ResourceType type, ImageView* imageview,
                                VkImageLayout layout, Sampler* sampler)
{
    switch(type)
    {
    case ResourceType::Sampler:
    case ResourceType::SampledImage:
    case ResourceType::CombineSamplerImage:
    case ResourceType::StorageImage:
    {
        auto& b                = m_commandState.resourceBindings.bindings;
        b[set][binding]->image = {
            .sampler     = sampler->getHandle(),
            .imageView   = imageview->getHandle(),
            .imageLayout = layout,
        };
    }
    break;
    default:
        APH_ASSERT(false);
        VK_LOG_ERR("Invalid resources type binding.");
        break;
    }
    m_commandState.resourceBindings.bindings[set][binding]->resType = utils::VkCast(type);
}
void CommandBuffer::setScissor(const VkExtent2D& extent)
{
    VkRect2D scissor = aph::vk::init::rect2D(extent);
    setScissor(scissor);
}
void CommandBuffer::setViewport(const VkExtent2D& extent)
{
    VkViewport viewport = aph::vk::init::viewport(extent);
    setViewport(viewport);
}
void CommandBuffer::beginDebugLabel(const DebugLabel& label)
{
    const VkDebugUtilsLabelEXT vkLabel = aph::vk::utils::VkCast(label);
    vkCmdBeginDebugUtilsLabelEXT(getHandle(), &vkLabel);
}
void CommandBuffer::insertDebugLabel(const DebugLabel& label)
{
    const VkDebugUtilsLabelEXT vkLabel = aph::vk::utils::VkCast(label);
    vkCmdInsertDebugUtilsLabelEXT(getHandle(), &vkLabel);
}
void CommandBuffer::endDebugLabel()
{
    vkCmdEndDebugUtilsLabelEXT(getHandle());
}
void CommandBuffer::insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers,
                                  const std::vector<ImageBarrier>&  pImageBarriers)
{
    uint32_t numTextureBarriers = pImageBarriers.size();
    uint32_t numBufferBarriers  = pBufferBarriers.size();

    VkImageMemoryBarrier* imageBarriers =
        (numTextureBarriers) ? (VkImageMemoryBarrier*)alloca((numTextureBarriers) * sizeof(VkImageMemoryBarrier))
                             : nullptr;
    uint32_t imageBarrierCount = 0;

    VkBufferMemoryBarrier* bufferBarriers =
        numBufferBarriers ? (VkBufferMemoryBarrier*)alloca(numBufferBarriers * sizeof(VkBufferMemoryBarrier)) : nullptr;
    uint32_t bufferBarrierCount = 0;

    VkAccessFlags srcAccessFlags = 0;
    VkAccessFlags dstAccessFlags = 0;

    for(uint32_t i = 0; i < numBufferBarriers; ++i)
    {
        const BufferBarrier*   pTrans         = &pBufferBarriers[i];
        Buffer*                pBuffer        = pTrans->pBuffer;
        VkBufferMemoryBarrier* pBufferBarrier = nullptr;

        if(RESOURCE_STATE_UNORDERED_ACCESS == pTrans->currentState &&
           RESOURCE_STATE_UNORDERED_ACCESS == pTrans->newState)
        {
            pBufferBarrier        = &bufferBarriers[bufferBarrierCount++];
            pBufferBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pBufferBarrier->pNext = nullptr;

            pBufferBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            pBufferBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        }
        else
        {
            pBufferBarrier        = &bufferBarriers[bufferBarrierCount++];
            pBufferBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pBufferBarrier->pNext = nullptr;

            pBufferBarrier->srcAccessMask = utils::getAccessFlags(pTrans->currentState);
            pBufferBarrier->dstAccessMask = utils::getAccessFlags(pTrans->newState);
        }

        if(pBufferBarrier)
        {
            pBufferBarrier->buffer = pBuffer->getHandle();
            pBufferBarrier->size   = VK_WHOLE_SIZE;
            pBufferBarrier->offset = 0;

            if(pTrans->acquire)
            {
                pBufferBarrier->srcQueueFamilyIndex = m_pDevice->getQueueByFlags(pTrans->queueType)->getFamilyIndex();
                pBufferBarrier->dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if(pTrans->release)
            {
                pBufferBarrier->srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                pBufferBarrier->dstQueueFamilyIndex = m_pDevice->getQueueByFlags(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                pBufferBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pBufferBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            srcAccessFlags |= pBufferBarrier->srcAccessMask;
            dstAccessFlags |= pBufferBarrier->dstAccessMask;
        }
    }

    for(uint32_t i = 0; i < numTextureBarriers; ++i)
    {
        const ImageBarrier*   pTrans        = &pImageBarriers[i];
        Image*                pImage        = pTrans->pImage;
        VkImageMemoryBarrier* pImageBarrier = nullptr;

        if(RESOURCE_STATE_UNORDERED_ACCESS == pTrans->currentState &&
           RESOURCE_STATE_UNORDERED_ACCESS == pTrans->newState)
        {
            pImageBarrier        = &imageBarriers[imageBarrierCount++];
            pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            pImageBarrier->pNext = nullptr;

            pImageBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            pImageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            pImageBarrier->oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
            pImageBarrier->newLayout     = VK_IMAGE_LAYOUT_GENERAL;
        }
        else
        {
            pImageBarrier        = &imageBarriers[imageBarrierCount++];
            pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            pImageBarrier->pNext = nullptr;

            pImageBarrier->srcAccessMask = utils::getAccessFlags(pTrans->currentState);
            pImageBarrier->dstAccessMask = utils::getAccessFlags(pTrans->newState);
            pImageBarrier->oldLayout     = utils::getImageLayout(pTrans->currentState);
            pImageBarrier->newLayout     = utils::getImageLayout(pTrans->newState);
        }

        if(pImageBarrier)
        {
            pImageBarrier->image                           = pImage->getHandle();
            pImageBarrier->subresourceRange.aspectMask     = utils::getImageAspect(utils::VkCast(pImage->getFormat()));
            pImageBarrier->subresourceRange.baseMipLevel   = pTrans->subresourceBarrier ? pTrans->mipLevel : 0;
            pImageBarrier->subresourceRange.levelCount     = pTrans->subresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
            pImageBarrier->subresourceRange.baseArrayLayer = pTrans->subresourceBarrier ? pTrans->arrayLayer : 0;
            pImageBarrier->subresourceRange.layerCount     = pTrans->subresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

            if(pTrans->acquire && pTrans->currentState != RESOURCE_STATE_UNDEFINED)
            {
                pImageBarrier->srcQueueFamilyIndex = m_pDevice->getQueueByFlags(pTrans->queueType)->getFamilyIndex();
                pImageBarrier->dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if(pTrans->release && pTrans->currentState != RESOURCE_STATE_UNDEFINED)
            {
                pImageBarrier->srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                pImageBarrier->dstQueueFamilyIndex = m_pDevice->getQueueByFlags(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            srcAccessFlags |= pImageBarrier->srcAccessMask;
            dstAccessFlags |= pImageBarrier->dstAccessMask;
        }

        pImage->m_resourceState = pTrans->newState;
        pImage->m_layout        = pImageBarrier->newLayout;
    }

    VkPipelineStageFlags srcStageMask = aph::vk::utils::determinePipelineStageFlags(
        m_pDevice->getPhysicalDevice(), srcAccessFlags, m_pQueue->getType());
    VkPipelineStageFlags dstStageMask = aph::vk::utils::determinePipelineStageFlags(
        m_pDevice->getPhysicalDevice(), dstAccessFlags, m_pQueue->getType());

    if(bufferBarrierCount || imageBarrierCount)
    {
        vkCmdPipelineBarrier(getHandle(), srcStageMask, dstStageMask, 0, 0, nullptr, bufferBarrierCount, bufferBarriers,
                             imageBarrierCount, imageBarriers);
    }
}
void CommandBuffer::transitionImageLayout(Image* pImage, ResourceState newState)
{
    aph::vk::ImageBarrier barrier{
        .pImage       = pImage,
        .currentState = pImage->getResourceState(),
        .newState     = newState,
    };
    insertBarrier({barrier});
}
}  // namespace aph::vk
