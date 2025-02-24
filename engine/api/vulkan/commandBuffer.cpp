#include "commandBuffer.h"
#include "api/vulkan/vkInit.h"
#include "device.h"

namespace aph::vk
{

CommandBuffer::CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue) :
    ResourceHandle(handle),
    m_pDevice(pDevice),
    m_pQueue(pQueue),
    m_pDeviceTable(pDevice->getDeviceTable()),
    m_state(RecordState::Initial)
{
}

CommandBuffer::~CommandBuffer() = default;

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    if(m_state == RecordState::Recording)
    {
        return VK_NOT_READY;
    }

    // Begin command recording.
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };
    auto result = m_pDeviceTable->vkBeginCommandBuffer(m_handle, &beginInfo);
    if(result != VK_SUCCESS)
    {
        return result;
    }

    // Mark CommandBuffer as recording and reset internal state.
    m_commandState = {};
    m_state        = RecordState::Recording;

    return VK_SUCCESS;
}

VkResult CommandBuffer::end()
{
    if(m_state != RecordState::Recording)
    {
        return VK_NOT_READY;
    }

    m_state = RecordState::Executable;

    return m_pDeviceTable->vkEndCommandBuffer(m_handle);
}

VkResult CommandBuffer::reset()
{
    if(m_handle != VK_NULL_HANDLE)
    {
        return m_pDeviceTable->vkResetCommandBuffer(m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }
    m_state = RecordState::Initial;
    return VK_SUCCESS;
}

void CommandBuffer::bindVertexBuffers(Buffer* pBuffer, uint32_t binding, std::size_t offset)
{
    APH_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS);

    VkBuffer vkBuffer                               = pBuffer->getHandle();
    m_commandState.graphics.vertex.buffers[binding] = vkBuffer;
    m_commandState.graphics.vertex.offsets[binding] = offset;
    m_commandState.graphics.vertex.dirty |= 1u << binding;
}

void CommandBuffer::bindIndexBuffers(Buffer* pBuffer, std::size_t offset, IndexType indexType)
{
    m_commandState.graphics.index = {
        .buffer = pBuffer->getHandle(), .offset = offset, .indexType = utils::VkCast(indexType)};
}

void CommandBuffer::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, MemoryRange range)
{
    VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = range.offset,
        .size      = range.size,
    };
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
void CommandBuffer::copyImage(Image* srcImage, Image* dstImage, VkExtent3D extent, const ImageCopyInfo& srcCopyInfo,
                              const ImageCopyInfo& dstCopyInfo)
{
    APH_ASSERT(srcImage && dstImage);
    if(extent.depth == 0 || extent.width == 0 || extent.height == 0)
    {
        APH_ASSERT(srcImage->getWidth() == dstImage->getWidth());
        APH_ASSERT(srcImage->getHeight() == dstImage->getHeight());
        APH_ASSERT(srcImage->getDepth() == dstImage->getDepth());

        extent = {srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth()};
    }

    // Copy region for transfer from framebuffer to cube face
    VkImageCopy copyRegion = {.srcSubresource = srcCopyInfo.subResources,
                              .srcOffset      = srcCopyInfo.offset,
                              .dstSubresource = dstCopyInfo.subResources,
                              .dstOffset      = dstCopyInfo.offset,
                              .extent         = extent};

    if(copyRegion.dstSubresource.aspectMask == VK_IMAGE_ASPECT_NONE)
    {
        copyRegion.dstSubresource.aspectMask = utils::getImageAspect(dstImage->getFormat());
    }
    if(copyRegion.srcSubresource.aspectMask == VK_IMAGE_ASPECT_NONE)
    {
        copyRegion.srcSubresource.aspectMask = utils::getImageAspect(srcImage->getFormat());
    }

    APH_ASSERT(copyRegion.srcSubresource.aspectMask == copyRegion.dstSubresource.aspectMask);

    copyRegion.extent.width  = srcImage->getWidth();
    copyRegion.extent.height = srcImage->getHeight();
    copyRegion.extent.depth  = 1;

    m_pDeviceTable->vkCmdCopyImage(m_handle, srcImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   dstImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}
void CommandBuffer::draw(DrawArguments args)
{
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDraw(m_handle, args.vertexCount, args.instanceCount, args.firstVertex, args.firstInstance);
}

void CommandBuffer::blitImage(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo,
                              const ImageBlitInfo& dstBlitInfo, VkFilter filter)
{
    const auto addOffset = [](const VkOffset3D& a, const VkOffset3D& b) -> VkOffset3D {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    };

    const auto isExtentValid = [](const VkOffset3D& extent) -> bool {
        return extent.x != 0 || extent.y != 0 || extent.z != 0;
    };

    VkImageBlit vkBlitInfo = {
        .srcSubresource = {.aspectMask     = utils::getImageAspect(srcImage->getFormat()),
                           .mipLevel       = srcBlitInfo.level,
                           .baseArrayLayer = srcBlitInfo.baseLayer,
                           .layerCount     = srcBlitInfo.layerCount},
        .dstSubresource = {.aspectMask     = utils::getImageAspect(dstImage->getFormat()),
                           .mipLevel       = dstBlitInfo.level,
                           .baseArrayLayer = dstBlitInfo.baseLayer,
                           .layerCount     = dstBlitInfo.layerCount},
    };

    vkBlitInfo.srcOffsets[0] = {srcBlitInfo.offset};

    if(isExtentValid(srcBlitInfo.extent))
    {
        vkBlitInfo.srcOffsets[1] = addOffset(srcBlitInfo.offset, srcBlitInfo.extent);
    }
    else
    {
        vkBlitInfo.srcOffsets[1] = {static_cast<int32_t>(srcImage->getWidth()),
                                    static_cast<int32_t>(srcImage->getHeight()), 1};
    }

    vkBlitInfo.dstOffsets[0] = {dstBlitInfo.offset};
    if(isExtentValid(dstBlitInfo.extent))
    {
        vkBlitInfo.dstOffsets[1] = addOffset(dstBlitInfo.offset, dstBlitInfo.extent);
    }
    else
    {
        vkBlitInfo.dstOffsets[1] = {static_cast<int32_t>(dstImage->getWidth()),
                                    static_cast<int32_t>(dstImage->getHeight()), 1};
    }

    m_pDeviceTable->vkCmdBlitImage(m_handle, srcImage->getHandle(), srcImage->m_layout, dstImage->getHandle(),
                                   dstImage->m_layout, 1, &vkBlitInfo, filter);
}

void CommandBuffer::endRendering()
{
    m_pDeviceTable->vkCmdEndRendering(getHandle());
}

void CommandBuffer::dispatch(DispatchArguments args)
{
    flushComputeCommand();
    m_pDeviceTable->vkCmdDispatch(getHandle(), args.x, args.y, args.z);
}
void CommandBuffer::dispatch(Buffer* pBuffer, std::size_t offset)
{
    flushComputeCommand();
    m_pDeviceTable->vkCmdDispatchIndirect(getHandle(), pBuffer->getHandle(), offset);
}
void CommandBuffer::draw(Buffer* pBuffer, std::size_t offset, uint32_t drawCount, uint32_t stride)
{
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDrawIndirect(getHandle(), pBuffer->getHandle(), offset, drawCount, stride);
}
void CommandBuffer::drawIndexed(DrawIndexArguments args)
{
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDrawIndexed(m_handle, args.indexCount, args.instanceCount, args.firstIndex, args.vertexOffset,
                                     args.firstInstance);
}

void CommandBuffer::beginRendering(const std::vector<Image*>& colors, Image* depth)
{
    RenderingInfo                renderingInfo;
    std::vector<AttachmentInfo>& colorAttachments = renderingInfo.colors;
    auto&                        depthAttachment  = renderingInfo.depth;
    colorAttachments.reserve(colors.size());
    for(auto color : colors)
    {
        colorAttachments.push_back({.image = color});
    }

    depthAttachment = {.image = depth};
    beginRendering(renderingInfo);
}
void CommandBuffer::beginRendering(const RenderingInfo& renderingInfo)
{
    m_commandState.graphics.color = renderingInfo.colors;
    m_commandState.graphics.depth = renderingInfo.depth;

    APH_ASSERT(!m_commandState.graphics.color.empty() || m_commandState.graphics.depth.image);

    SmallVector<VkRenderingAttachmentInfo> vkColors;
    VkRenderingAttachmentInfo              vkDepth;
    SmallVector<VkViewport>                vkViewports;
    SmallVector<VkRect2D>                  vkScissors;
    vkColors.reserve(m_commandState.graphics.color.size());
    for(const auto& color : m_commandState.graphics.color)
    {
        auto&                     image = color.image;
        VkRenderingAttachmentInfo vkColorAttrInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = image->getView()->getHandle(),
            .imageLayout = color.layout.value_or(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            .loadOp      = color.loadOp.value_or(VK_ATTACHMENT_LOAD_OP_CLEAR),
            .storeOp     = color.storeOp.value_or(VK_ATTACHMENT_STORE_OP_STORE),
            .clearValue  = color.clear.value_or(VkClearValue{.color{{0.0f, 0.0f, 0.0f, 1.0f}}})};
        vkColors.push_back(vkColorAttrInfo);

        VkRect2D   renderArea = {.offset = {0, 0}, .extent = {color.image->getWidth(), color.image->getHeight()}};
        VkViewport viewPort   = aph::vk::init::viewport(renderArea.extent);
        vkScissors.push_back(renderArea);
        vkViewports.push_back(viewPort);
    }

    m_pDeviceTable->vkCmdSetViewportWithCount(m_handle, vkViewports.size(), vkViewports.data());
    m_pDeviceTable->vkCmdSetScissorWithCount(m_handle, vkScissors.size(), vkScissors.data());

    VkRenderingInfo vkRenderingInfo{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = vkScissors[0],
        .layerCount           = 1,
        .colorAttachmentCount = static_cast<uint32_t>(vkColors.size()),
        .pColorAttachments    = vkColors.data(),
        .pDepthAttachment     = nullptr,
    };

    if(const auto& depth = m_commandState.graphics.depth; depth.image != nullptr)
    {
        vkDepth = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = depth.image->getView()->getHandle(),
            .imageLayout = depth.layout.value_or(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL),
            .loadOp      = depth.loadOp.value_or(VK_ATTACHMENT_LOAD_OP_CLEAR),
            .storeOp     = depth.storeOp.value_or(VK_ATTACHMENT_STORE_OP_DONT_CARE),
            .clearValue  = depth.clear.value_or(VkClearValue{.depthStencil{1.0f, 0x00}}),
        };
        vkRenderingInfo.pDepthAttachment = &vkDepth;
    }
    m_pDeviceTable->vkCmdBeginRendering(getHandle(), &vkRenderingInfo);
}

void CommandBuffer::flushComputeCommand()
{
    auto pProgram = m_commandState.pProgram;
    APH_ASSERT(pProgram);
    APH_ASSERT(pProgram->getPipelineType() == PipelineType::Compute);

    SmallVector<VkShaderStageFlagBits> stages     = {VK_SHADER_STAGE_COMPUTE_BIT};
    SmallVector<VkShaderEXT>           shaderObjs = {pProgram->getShaderObject(ShaderStage::CS)};
    m_pDeviceTable->vkCmdBindShadersEXT(getHandle(), stages.size(), stages.data(), shaderObjs.data());

    flushDescriptorSet();
}

void CommandBuffer::flushGraphicsCommand()
{
    initDynamicGraphicsState();

    // shader object binding
    {
        const auto& vertexState = m_commandState.graphics.vertex;
        const auto& indexState  = m_commandState.graphics.index;
        const auto& pProgram    = m_commandState.pProgram;
        APH_ASSERT(pProgram);

        enum
        {
            VS  = 0,
            FS  = 1,
            TCS = 2,
            TES = 3,
            GS  = 4,
            TS  = 5,
            MS  = 6,
            NUM_STAGE
        };
        const std::array<VkShaderStageFlagBits, NUM_STAGE> stages = {VK_SHADER_STAGE_VERTEX_BIT,
                                                                     VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                     VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                                                                     VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                                                                     VK_SHADER_STAGE_GEOMETRY_BIT,
                                                                     VK_SHADER_STAGE_TASK_BIT_EXT,
                                                                     VK_SHADER_STAGE_MESH_BIT_EXT};
        std::array<VkShaderEXT, NUM_STAGE>                 shaderObjs{};

        if(pProgram->getPipelineType() == PipelineType::Geometry)
        {
            shaderObjs[VS] = pProgram->getShaderObject(ShaderStage::VS);
            shaderObjs[FS] = pProgram->getShaderObject(ShaderStage::FS);

            SmallVector<VkVertexInputBindingDescription2EXT>   vkBindings;
            SmallVector<VkVertexInputAttributeDescription2EXT> vkAttributes;

            {
                const VertexInput& vstate = vertexState.inputInfo.value_or(pProgram->getVertexInput());

                vkAttributes.resize(vstate.attributes.size());
                SmallVector<bool> bufferAlreadyBound(vstate.bindings.size());

                for(uint32_t i = 0; i != vkAttributes.size(); i++)
                {
                    const auto& attr = vstate.attributes[i];

                    vkAttributes[i] = {.sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
                                       .location = attr.location,
                                       .binding  = attr.binding,
                                       .format   = utils::VkCast(attr.format),
                                       .offset   = (uint32_t)attr.offset};

                    if(!bufferAlreadyBound[attr.binding])
                    {
                        bufferAlreadyBound[attr.binding] = true;
                        vkBindings.push_back({.sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
                                              .binding   = attr.binding,
                                              .stride    = vstate.bindings[attr.binding].stride,
                                              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                                              .divisor   = 1});
                    }
                }
            }
            m_pDeviceTable->vkCmdSetVertexInputEXT(getHandle(), vkBindings.size(), vkBindings.data(),
                                                   vkAttributes.size(), vkAttributes.data());
            m_pDeviceTable->vkCmdSetPrimitiveTopology(getHandle(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            m_pDeviceTable->vkCmdSetPrimitiveRestartEnable(getHandle(), VK_FALSE);

            aph::utils::forEachBitRange(vertexState.dirty, [&](uint32_t binding, uint32_t bindingCount) {
                m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, binding, bindingCount, vertexState.buffers + binding,
                                                       vertexState.offsets + binding);
            });

            m_pDeviceTable->vkCmdBindVertexBuffers(m_handle, 0, 1, &vertexState.buffers[0], vertexState.offsets);
            m_pDeviceTable->vkCmdBindIndexBuffer(m_handle, indexState.buffer, indexState.offset, indexState.indexType);
        }
        else if(pProgram->getPipelineType() == PipelineType::Mesh)
        {
            shaderObjs[TS] = pProgram->getShaderObject(ShaderStage::TS);
            shaderObjs[MS] = pProgram->getShaderObject(ShaderStage::MS);
            shaderObjs[FS] = pProgram->getShaderObject(ShaderStage::FS);
        }
        else
        {
            APH_ASSERT(false);
            CM_LOG_ERR("Invalid pipeline type.");
        }

        m_pDeviceTable->vkCmdBindShadersEXT(getHandle(), stages.size(), stages.data(), shaderObjs.data());
    }

    flushDescriptorSet();
}

void CommandBuffer::beginDebugLabel(const DebugLabel& label)
{
#ifdef APH_DEBUG
    const VkDebugUtilsLabelEXT vkLabel = aph::vk::utils::VkCast(label);
    vkCmdBeginDebugUtilsLabelEXT(getHandle(), &vkLabel);
#endif
}
void CommandBuffer::insertDebugLabel(const DebugLabel& label)
{
#ifdef APH_DEBUG
    const VkDebugUtilsLabelEXT vkLabel = aph::vk::utils::VkCast(label);
    vkCmdInsertDebugUtilsLabelEXT(getHandle(), &vkLabel);
#endif
}
void CommandBuffer::endDebugLabel()
{
#ifdef APH_DEBUG
    vkCmdEndDebugUtilsLabelEXT(getHandle());
#endif
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

        if(ResourceState::UnorderedAccess == pTrans->currentState && ResourceState::UnorderedAccess == pTrans->newState)
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
                pBufferBarrier->srcQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
                pBufferBarrier->dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if(pTrans->release)
            {
                pBufferBarrier->srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                pBufferBarrier->dstQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                pBufferBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pBufferBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            srcAccessFlags |= pBufferBarrier->srcAccessMask;
            dstAccessFlags |= pBufferBarrier->dstAccessMask;

            pBuffer->m_resourceState = pTrans->newState;
        }
    }

    for(uint32_t i = 0; i < numTextureBarriers; ++i)
    {
        const ImageBarrier*   pTrans        = &pImageBarriers[i];
        Image*                pImage        = pTrans->pImage;
        VkImageMemoryBarrier* pImageBarrier = nullptr;

        if(ResourceState::UnorderedAccess == pTrans->currentState && ResourceState::UnorderedAccess == pTrans->newState)
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
            pImageBarrier->subresourceRange.aspectMask     = utils::getImageAspect(pImage->getFormat());
            pImageBarrier->subresourceRange.baseMipLevel   = pTrans->subresourceBarrier ? pTrans->mipLevel : 0;
            pImageBarrier->subresourceRange.levelCount     = pTrans->subresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
            pImageBarrier->subresourceRange.baseArrayLayer = pTrans->subresourceBarrier ? pTrans->arrayLayer : 0;
            pImageBarrier->subresourceRange.layerCount     = pTrans->subresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

            if(pTrans->acquire && pTrans->currentState != ResourceState::Undefined)
            {
                pImageBarrier->srcQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
                pImageBarrier->dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if(pTrans->release && pTrans->currentState != ResourceState::Undefined)
            {
                pImageBarrier->srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                pImageBarrier->dstQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            srcAccessFlags |= pImageBarrier->srcAccessMask;
            dstAccessFlags |= pImageBarrier->dstAccessMask;

            pImage->m_resourceState = pTrans->newState;
            pImage->m_layout        = pImageBarrier->newLayout;
        }
    }

    VkPipelineStageFlags srcStageMask = m_pDevice->determinePipelineStageFlags(srcAccessFlags, m_pQueue->getType());
    VkPipelineStageFlags dstStageMask = m_pDevice->determinePipelineStageFlags(dstAccessFlags, m_pQueue->getType());

    if(bufferBarrierCount || imageBarrierCount)
    {
        m_pDeviceTable->vkCmdPipelineBarrier(getHandle(), srcStageMask, dstStageMask, 0, 0, nullptr, bufferBarrierCount,
                                             bufferBarriers, imageBarrierCount, imageBarriers);
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
void CommandBuffer::resetQueryPool(VkQueryPool pool, uint32_t first, uint32_t count)
{
    m_pDeviceTable->vkCmdResetQueryPool(getHandle(), pool, first, count);
}
void CommandBuffer::writeTimeStamp(VkPipelineStageFlagBits stage, VkQueryPool pool, uint32_t queryIndex)
{
    m_pDeviceTable->vkCmdWriteTimestamp(getHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, queryIndex);
}
void CommandBuffer::setDebugName(std::string_view debugName)
{
    utils::setDebugObjectName(m_pDevice->getHandle(), VK_OBJECT_TYPE_COMMAND_BUFFER, uint64_t(getHandle()), debugName);
}
void CommandBuffer::updateBuffer(Buffer* pBuffer, MemoryRange range, const void* data)
{
    m_pDeviceTable->vkCmdUpdateBuffer(getHandle(), pBuffer->getHandle(), range.offset, range.size, data);
}
void CommandBuffer::setResource(const std::vector<Sampler*>& samplers, uint32_t set, uint32_t binding)
{
    auto& resBindings = m_commandState.resourceBindings;

    DescriptorUpdateInfo newUpdate = {
        .binding  = binding,
        .samplers = samplers,
    };

    if(resBindings.bindings[set][binding] != newUpdate)
    {
        resBindings.bindings[set][binding] = std::move(newUpdate);
        resBindings.setBit |= 1u << set;
        resBindings.setBindingBit[set] |= 1u << binding;
        resBindings.dirtyBinding[set] |= 1u << binding;
    }
}
void CommandBuffer::setResource(const std::vector<Image*>& images, uint32_t set, uint32_t binding)
{
    auto& resBindings = m_commandState.resourceBindings;

    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .images  = images,
    };

    if(resBindings.bindings[set][binding] != newUpdate)
    {
        resBindings.bindings[set][binding] = std::move(newUpdate);
        resBindings.setBit |= 1u << set;
        resBindings.setBindingBit[set] |= 1u << binding;
        resBindings.dirtyBinding[set] |= 1u << binding;
    }
}
void CommandBuffer::setResource(const std::vector<Buffer*>& buffers, uint32_t set, uint32_t binding)
{
    auto& resBindings = m_commandState.resourceBindings;

    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .buffers = buffers,
    };

    if(resBindings.bindings[set][binding] != newUpdate)
    {
        resBindings.bindings[set][binding] = std::move(newUpdate);
        resBindings.setBit |= 1u << set;
        resBindings.setBindingBit[set] |= 1u << binding;
        resBindings.dirtyBinding[set] |= 1u << binding;
    }
}
void CommandBuffer::setDepthState(const DepthState& state)
{
    m_commandState.graphics.depthState = state;
}
void CommandBuffer::draw(DispatchArguments args)
{
    flushGraphicsCommand();
    m_pDeviceTable->vkCmdDrawMeshTasksEXT(m_handle, args.x, args.y, args.z);
}

void CommandBuffer::initDynamicGraphicsState()
{
    {
        auto&             state = m_commandState.graphics.depthState;
        const VkCompareOp op    = utils::VkCast(state.compareOp);
        m_pDeviceTable->vkCmdSetDepthWriteEnable(getHandle(), state.enableWrite ? VK_TRUE : VK_FALSE);
        m_pDeviceTable->vkCmdSetDepthTestEnable(getHandle(), op != VK_COMPARE_OP_ALWAYS);
        m_pDeviceTable->vkCmdSetDepthCompareOp(getHandle(), op);
        m_pDeviceTable->vkCmdSetDepthClampEnableEXT(getHandle(), VK_FALSE);
        m_pDeviceTable->vkCmdSetCullModeEXT(getHandle(), utils::VkCast(m_commandState.graphics.cullMode));
        m_pDeviceTable->vkCmdSetAlphaToOneEnableEXT(getHandle(), VK_FALSE);
    }

    {
        VkColorBlendEquationEXT colorBlendEquationEXT{};
        m_pDeviceTable->vkCmdSetColorBlendEquationEXT(getHandle(), 0, 1, &colorBlendEquationEXT);
    }

    m_pDeviceTable->vkCmdSetRasterizerDiscardEnable(getHandle(), VK_FALSE);

    m_pDeviceTable->vkCmdSetRasterizationSamplesEXT(getHandle(), VK_SAMPLE_COUNT_1_BIT);

    {
        // Use 1 sample per pixel
        const VkSampleMask sample_mask = 0x1;
        m_pDeviceTable->vkCmdSetSampleMaskEXT(getHandle(), VK_SAMPLE_COUNT_1_BIT, &sample_mask);
    }

    // Do not use alpha to coverage or alpha to one because not using MSAA
    m_pDeviceTable->vkCmdSetAlphaToCoverageEnableEXT(getHandle(), VK_FALSE);

    bool wireframeEnabled = m_commandState.graphics.polygonMode == PolygonMode::Line;
    m_pDeviceTable->vkCmdSetPolygonModeEXT(getHandle(), wireframeEnabled ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
    if(wireframeEnabled)
    {
        m_pDeviceTable->vkCmdSetLineWidth(getHandle(), 1.0f);
    }

    // Set front face, cull mode is set in build_command_buffers.
    m_pDeviceTable->vkCmdSetFrontFaceEXT(getHandle(), VK_FRONT_FACE_COUNTER_CLOCKWISE);

    // Set depth state, the depth write. Don't enable depth bounds, bias, or stencil test.
    m_pDeviceTable->vkCmdSetDepthTestEnableEXT(getHandle(), VK_TRUE);
    m_pDeviceTable->vkCmdSetDepthCompareOpEXT(getHandle(), VK_COMPARE_OP_GREATER);
    m_pDeviceTable->vkCmdSetDepthBoundsTestEnableEXT(getHandle(), VK_FALSE);
    m_pDeviceTable->vkCmdSetDepthBiasEnableEXT(getHandle(), VK_FALSE);
    m_pDeviceTable->vkCmdSetStencilTestEnableEXT(getHandle(), VK_FALSE);

    // Do not enable logic op
    m_pDeviceTable->vkCmdSetLogicOpEnableEXT(getHandle(), VK_FALSE);

    {
        // Disable color blending
        VkBool32 color_blend_enables[] = {VK_FALSE};
        m_pDeviceTable->vkCmdSetColorBlendEnableEXT(getHandle(), 0, 1, color_blend_enables);
    }

    {
        // Use RGBA color write mask
        VkColorComponentFlags color_component_flags[] = {VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT |
                                                         VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT};
        m_pDeviceTable->vkCmdSetColorWriteMaskEXT(getHandle(), 0, 1, color_component_flags);
    }
}
void CommandBuffer::flushDescriptorSet()
{
    aph::utils::forEachBit(m_commandState.resourceBindings.setBit, [this](uint32_t setIdx) {
        APH_ASSERT(setIdx < VULKAN_NUM_DESCRIPTOR_SETS);
        aph::utils::forEachBit(m_commandState.resourceBindings.setBindingBit[setIdx], [this, setIdx](auto bindingIdx) {
            if(!(m_commandState.resourceBindings.dirtyBinding[setIdx] & (1u << bindingIdx)))
            {
                CM_LOG_INFO("skip update");
                return;
            }
            auto& set = m_commandState.resourceBindings.sets[setIdx];
            if(set == nullptr)
            {
                set = m_commandState.pProgram->getSetLayout(setIdx)->allocateSet();
            }
            set->update(m_commandState.resourceBindings.bindings[setIdx][bindingIdx]);
            m_commandState.resourceBindings.sets[setIdx] = set;
        });
        m_commandState.resourceBindings.dirtyBinding[setIdx] = 0;
    });

    aph::utils::forEachBit(m_commandState.resourceBindings.setBit, [this](uint32_t setIndex) {
        auto& set = m_commandState.resourceBindings.sets[setIndex];
        m_pDeviceTable->vkCmdBindDescriptorSets(m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                m_commandState.pProgram->getPipelineLayout(), setIndex, 1,
                                                &set->getHandle(), 0, nullptr);
    });
}
}  // namespace aph::vk
