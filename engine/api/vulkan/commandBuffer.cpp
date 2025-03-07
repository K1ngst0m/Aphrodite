#include "commandBuffer.h"
#include "bindless.h"
#include "device.h"

namespace aph::vk
{

CommandBuffer::CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue)
    : ResourceHandle(handle)
    , m_pDevice(pDevice)
    , m_pQueue(pQueue)
    , m_state(RecordState::Initial)
{
}

CommandBuffer::~CommandBuffer() = default;

Result CommandBuffer::begin(::vk::CommandBufferUsageFlags flags)
{
    if (m_state == RecordState::Recording)
    {
        return { Result::RuntimeError, "Command buffer is not ready." };
    }

    // Begin command recording.
    ::vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.setFlags(flags);

    auto result = getHandle().begin(beginInfo);
    if (result != ::vk::Result::eSuccess)
    {
        return utils::getResult(result);
    }

    // Mark CommandBuffer as recording and reset internal state.
    m_commandState = {};
    m_state = RecordState::Recording;

    return Result::Success;
}

Result CommandBuffer::end()
{
    if (m_state != RecordState::Recording)
    {
        return { Result::RuntimeError, "Commands are not recorded yet" };
    }

    m_state = RecordState::Executable;

    return utils::getResult(getHandle().end());
}

Result CommandBuffer::reset()
{
    if (m_handle != VK_NULL_HANDLE)
    {
        getHandle().reset(::vk::CommandBufferResetFlagBits::eReleaseResources);
    }
    m_state = RecordState::Initial;
    return Result::Success;
}

void CommandBuffer::bindVertexBuffers(Buffer* pBuffer, uint32_t binding, std::size_t offset)
{
    APH_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS);

    auto& vertexState = m_commandState.graphics.vertex;
    vertexState.buffers[binding] = pBuffer->getHandle();
    vertexState.offsets[binding] = offset;
    setDirty(DirtyFlagBits::vertexState);
    vertexState.dirty.set(binding);
}

void CommandBuffer::bindIndexBuffers(Buffer* pBuffer, std::size_t offset, IndexType indexType)
{
    auto& indexState = m_commandState.graphics.index;
    indexState.buffer = pBuffer->getHandle();
    indexState.offset = offset;
    indexState.indexType = indexType;
    setDirty(DirtyFlagBits::indexState);
}

void CommandBuffer::copy(Buffer* srcBuffer, Buffer* dstBuffer, Range range)
{
    ::vk::BufferCopy copyRegion{};
    copyRegion.setSize(range.size).setSrcOffset(0).setDstOffset(range.offset);
    getHandle().copyBuffer(srcBuffer->getHandle(), dstBuffer->getHandle(), { copyRegion });
}

void CommandBuffer::copy(Buffer* buffer, Image* image, ArrayProxy<::vk::BufferImageCopy> regions)
{
    if (regions.empty())
    {
        ::vk::BufferImageCopy region{};

        region.imageSubresource.setLayerCount(1).setAspectMask(::vk::ImageAspectFlagBits::eColor);
        region.imageExtent = ::vk::Extent3D{ image->getWidth(), image->getHeight(), 1 };
        getHandle().copyBufferToImage(buffer->getHandle(), image->getHandle(), ::vk::ImageLayout::eTransferDstOptimal,
                                      { region });
    }
    else
    {
        getHandle().copyBufferToImage(buffer->getHandle(), image->getHandle(), ::vk::ImageLayout::eTransferDstOptimal,
                                      regions);
    }
}

void CommandBuffer::copy(Image* srcImage, Image* dstImage, Extent3D extent, const ImageCopyInfo& srcCopyInfo,
                         const ImageCopyInfo& dstCopyInfo)
{
    APH_ASSERT(srcImage && dstImage);
    if (extent.depth == 0 || extent.width == 0 || extent.height == 0)
    {
        APH_ASSERT(srcImage->getWidth() == dstImage->getWidth());
        APH_ASSERT(srcImage->getHeight() == dstImage->getHeight());
        APH_ASSERT(srcImage->getDepth() == dstImage->getDepth());

        extent = { srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth() };
    }

    // Copy region for transfer from framebuffer to cube face
    ::vk::ImageCopy copyRegion{};
    copyRegion.setExtent(::vk::Extent3D{ extent.width, extent.height, extent.depth })
        .setSrcOffset(srcCopyInfo.offset)
        .setSrcSubresource(srcCopyInfo.subResources)
        .setDstOffset(dstCopyInfo.offset)
        .setDstSubresource(dstCopyInfo.subResources);

    if (copyRegion.dstSubresource.aspectMask == ::vk::ImageAspectFlagBits::eNone)
    {
        copyRegion.dstSubresource.aspectMask = utils::getImageAspect(dstImage->getFormat());
    }
    if (copyRegion.srcSubresource.aspectMask == ::vk::ImageAspectFlagBits::eNone)
    {
        copyRegion.srcSubresource.aspectMask = utils::getImageAspect(srcImage->getFormat());
    }

    APH_ASSERT(copyRegion.srcSubresource.aspectMask == copyRegion.dstSubresource.aspectMask);

    copyRegion.extent.width = srcImage->getWidth();
    copyRegion.extent.height = srcImage->getHeight();
    copyRegion.extent.depth = 1;

    getHandle().copyImage(srcImage->getHandle(), ::vk::ImageLayout::eTransferSrcOptimal, dstImage->getHandle(),
                          ::vk::ImageLayout::eTransferDstOptimal, { copyRegion });
}
void CommandBuffer::draw(DrawArguments args)
{
    flushGraphicsCommand();
    getHandle().draw(args.vertexCount, args.instanceCount, args.firstVertex, args.firstInstance);
}

void CommandBuffer::blit(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo,
                         const ImageBlitInfo& dstBlitInfo, ::vk::Filter filter)
{
    const auto addOffset = [](const ::vk::Offset3D& a, const ::vk::Offset3D& b) -> ::vk::Offset3D
    { return { a.x + b.x, a.y + b.y, a.z + b.z }; };

    const auto isExtentValid = [](const ::vk::Offset3D& extent) -> bool
    { return extent.x != 0 || extent.y != 0 || extent.z != 0; };

    ::vk::ImageBlit vkBlitInfo{};

    ::vk::ImageSubresourceLayers srcSubResource{};
    srcSubResource.setMipLevel(srcBlitInfo.level)
        .setBaseArrayLayer(srcBlitInfo.baseLayer)
        .setLayerCount(srcBlitInfo.layerCount)
        .setAspectMask(utils::getImageAspect(srcImage->getFormat()));

    ::vk::ImageSubresourceLayers dstSubResource{};
    dstSubResource.setMipLevel(dstBlitInfo.level)
        .setBaseArrayLayer(dstBlitInfo.baseLayer)
        .setLayerCount(dstBlitInfo.layerCount)
        .setAspectMask(utils::getImageAspect(dstImage->getFormat()));

    vkBlitInfo.setSrcSubresource(srcSubResource).setDstSubresource(dstSubResource);

    vkBlitInfo.srcOffsets[0] = { srcBlitInfo.offset };

    if (isExtentValid(srcBlitInfo.extent))
    {
        vkBlitInfo.srcOffsets[1] = addOffset(srcBlitInfo.offset, srcBlitInfo.extent);
    }
    else
    {
        vkBlitInfo.srcOffsets[1] = ::vk::Offset3D{ static_cast<int32_t>(srcImage->getWidth()),
                                                   static_cast<int32_t>(srcImage->getHeight()), 1 };
    }

    vkBlitInfo.dstOffsets[0] = { dstBlitInfo.offset };
    if (isExtentValid(dstBlitInfo.extent))
    {
        vkBlitInfo.dstOffsets[1] = addOffset(dstBlitInfo.offset, dstBlitInfo.extent);
    }
    else
    {
        vkBlitInfo.dstOffsets[1] = ::vk::Offset3D{ static_cast<int32_t>(dstImage->getWidth()),
                                                   static_cast<int32_t>(dstImage->getHeight()), 1 };
    }

    getHandle().blitImage(srcImage->getHandle(), srcImage->m_layout, dstImage->getHandle(), dstImage->m_layout,
                          { vkBlitInfo }, filter);
}

void CommandBuffer::endRendering()
{
    getHandle().endRendering();
}

void CommandBuffer::dispatch(DispatchArguments args)
{
    flushComputeCommand();
    getHandle().dispatch(args.x, args.y, args.z);
}
void CommandBuffer::dispatch(Buffer* pBuffer, std::size_t offset)
{
    flushComputeCommand();
    getHandle().dispatchIndirect(pBuffer->getHandle(), offset);
}
void CommandBuffer::draw(Buffer* pBuffer, std::size_t offset, uint32_t drawCount, uint32_t stride)
{
    flushGraphicsCommand();
    getHandle().drawIndirect(pBuffer->getHandle(), offset, drawCount, stride);
}
void CommandBuffer::drawIndexed(DrawIndexArguments args)
{
    flushGraphicsCommand();
    getHandle().drawIndexed(args.indexCount, args.instanceCount, args.firstIndex, args.vertexOffset,
                            args.firstInstance);
}

void CommandBuffer::beginRendering(ArrayProxy<Image*> colors, Image* depth)
{
    RenderingInfo renderingInfo;
    auto& colorAttachments = renderingInfo.colors;
    auto& depthAttachment = renderingInfo.depth;
    colorAttachments.reserve(colors.size());
    for (auto color : colors)
    {
        colorAttachments.push_back({ .image = color });
    }

    depthAttachment = { .image = depth };
    beginRendering(renderingInfo);
}
void CommandBuffer::beginRendering(const RenderingInfo& renderingInfo)
{
    m_commandState.graphics.color = renderingInfo.colors;
    m_commandState.graphics.depth = renderingInfo.depth;

    APH_ASSERT(!m_commandState.graphics.color.empty() || m_commandState.graphics.depth.image);

    SmallVector<::vk::RenderingAttachmentInfo> vkColors;
    SmallVector<::vk::Viewport> vkViewports;
    SmallVector<::vk::Rect2D> vkScissors;
    vkColors.reserve(m_commandState.graphics.color.size());
    for (const auto& color : m_commandState.graphics.color)
    {
        auto& image = color.image;
        ::vk::RenderingAttachmentInfo vkColorAttrInfo{};
        vkColorAttrInfo.setImageView(image->getView()->getHandle())
            .setImageLayout(color.layout.value_or(::vk::ImageLayout::eColorAttachmentOptimal))
            .setLoadOp(color.loadOp.value_or(::vk::AttachmentLoadOp::eClear))
            .setStoreOp(color.storeOp.value_or(::vk::AttachmentStoreOp::eStore))
            .setClearValue(color.clear.value_or(::vk::ClearValue{}.setColor({ 0.0f, 0.0f, 0.0f, 1.0f })));

        vkColors.push_back(vkColorAttrInfo);

        ::vk::Rect2D renderArea{};
        renderArea.setExtent({ color.image->getWidth(), color.image->getHeight() });
        ::vk::Viewport viewPort{};
        viewPort.setMaxDepth(1.0f).setWidth(renderArea.extent.width).setHeight(renderArea.extent.height);

        vkScissors.push_back(renderArea);
        vkViewports.push_back(viewPort);
    }

    getHandle().setViewportWithCount(vkViewports);
    getHandle().setScissorWithCount(vkScissors);

    ::vk::RenderingInfo vkRenderingInfo{};
    vkRenderingInfo.setRenderArea(vkScissors[0]).setLayerCount(1).setColorAttachments(vkColors);

    ::vk::RenderingAttachmentInfo vkDepth;
    if (const auto& depth = m_commandState.graphics.depth; depth.image != nullptr)
    {
        vkDepth.setImageView(depth.image->getView()->getHandle())
            .setImageLayout(depth.layout.value_or(::vk::ImageLayout::eDepthAttachmentOptimal))
            .setLoadOp(depth.loadOp.value_or(::vk::AttachmentLoadOp::eClear))
            .setStoreOp(depth.storeOp.value_or(::vk::AttachmentStoreOp::eDontCare))
            .setClearValue(depth.clear.value_or(::vk::ClearValue{}.setDepthStencil({ 1.0f, 0x00 })));

        vkRenderingInfo.setPDepthAttachment(&vkDepth);
    }
    getHandle().beginRendering(vkRenderingInfo);
}

void CommandBuffer::flushComputeCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset)
{
    auto pProgram = m_commandState.pProgram;
    APH_ASSERT(pProgram);
    APH_ASSERT(pProgram->getPipelineType() == PipelineType::Compute);
    SmallVector<::vk::ShaderStageFlagBits> stages = { ::vk::ShaderStageFlagBits::eCompute };
    SmallVector<::vk::ShaderEXT> shaderObjs = { pProgram->getShaderObject(ShaderStage::CS) };
    getHandle().bindShadersEXT(stages, shaderObjs);
    flushDescriptorSet(dynamicOffset);
    m_commandState.dirty = {};
}

void CommandBuffer::flushGraphicsCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset)
{
    flushDynamicGraphicsState();

    // shader object binding
    {
        const auto& vertexInput = m_commandState.graphics.vertexInput;
        auto& vertexState = m_commandState.graphics.vertex;
        auto& indexState = m_commandState.graphics.index;
        const auto& pProgram = m_commandState.pProgram;
        APH_ASSERT(pProgram);

        enum
        {
            VS = 0,
            FS,
            TCS,
            TES,
            GS,
            TS,
            MS,
            NUM_STAGE
        };

        constexpr std::array<::vk::ShaderStageFlagBits, NUM_STAGE> stages = {
            ::vk::ShaderStageFlagBits::eVertex,
            ::vk::ShaderStageFlagBits::eFragment,
            ::vk::ShaderStageFlagBits::eTessellationControl,
            ::vk::ShaderStageFlagBits::eTessellationEvaluation,
            ::vk::ShaderStageFlagBits::eGeometry,
            ::vk::ShaderStageFlagBits::eTaskEXT,
            ::vk::ShaderStageFlagBits::eMeshEXT
        };
        std::array<::vk::ShaderEXT, NUM_STAGE> shaderObjs{};

        if (pProgram->getPipelineType() == PipelineType::Geometry)
        {
            shaderObjs[VS] = pProgram->getShaderObject(ShaderStage::VS);
            shaderObjs[FS] = pProgram->getShaderObject(ShaderStage::FS);

            SmallVector<::vk::VertexInputBindingDescription2EXT> vkBindings;
            SmallVector<::vk::VertexInputAttributeDescription2EXT> vkAttributes;

            if (m_commandState.dirty & DirtyFlagBits::vertexInput)
            {
                const VertexInput& vstate = vertexInput.value_or(pProgram->getVertexInput());

                vkAttributes.resize(vstate.attributes.size());
                SmallVector<bool> bufferAlreadyBound(vstate.bindings.size());

                for (uint32_t i = 0; i != vkAttributes.size(); i++)
                {
                    const auto& attr = vstate.attributes[i];

                    vkAttributes[i]
                        .setLocation(attr.location)
                        .setBinding(attr.binding)
                        .setFormat(utils::VkCast(attr.format))
                        .setOffset(attr.offset);

                    if (!bufferAlreadyBound[attr.binding])
                    {
                        bufferAlreadyBound[attr.binding] = true;
                        vkBindings.emplace_back()
                            .setBinding(attr.binding)
                            .setStride(vstate.bindings[attr.binding].stride)
                            .setInputRate(::vk::VertexInputRate::eVertex)
                            .setDivisor(1);
                    }
                }

                getHandle().setVertexInputEXT(vkBindings, vkAttributes);
                getHandle().setPrimitiveTopology(utils::VkCast(m_commandState.graphics.topology));
                getHandle().setPrimitiveRestartEnable(::vk::False);
            }

            if (m_commandState.dirty & DirtyFlagBits::vertexState)
            {
                aph::utils::forEachBitRange(vertexState.dirty,
                                            [&](uint32_t binding, uint32_t bindingCount)
                                            {
                                                getHandle().bindVertexBuffers(binding, bindingCount,
                                                                              vertexState.buffers + binding,
                                                                              vertexState.offsets + binding);
                                            });
                vertexState.dirty.reset();
            }

            if (m_commandState.dirty & DirtyFlagBits::indexState)
            {
                getHandle().bindIndexBuffer(indexState.buffer, indexState.offset, utils::VkCast(indexState.indexType));
            }
        }
        else if (pProgram->getPipelineType() == PipelineType::Mesh)
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

        getHandle().bindShadersEXT(stages, shaderObjs);
    }

    flushDescriptorSet(dynamicOffset);
    m_commandState.dirty = {};
}

void CommandBuffer::beginDebugLabel(const DebugLabel& label)
{
#ifdef APH_DEBUG
    const ::vk::DebugUtilsLabelEXT vkLabel = utils::VkCast(label);
    getHandle().beginDebugUtilsLabelEXT(vkLabel);
#endif
}
void CommandBuffer::insertDebugLabel(const DebugLabel& label)
{
#ifdef APH_DEBUG
    const ::vk::DebugUtilsLabelEXT vkLabel = utils::VkCast(label);
    getHandle().insertDebugUtilsLabelEXT(vkLabel);
#endif
}
void CommandBuffer::endDebugLabel()
{
#ifdef APH_DEBUG
    getHandle().endDebugUtilsLabelEXT();
#endif
}
void CommandBuffer::insertBarrier(ArrayProxy<ImageBarrier> pImageBarriers)
{
    insertBarrier({}, pImageBarriers);
}
void CommandBuffer::insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers)
{
    insertBarrier(pBufferBarriers, {});
}
void CommandBuffer::insertBarrier(ArrayProxy<BufferBarrier> bufferBarriers, ArrayProxy<ImageBarrier> imageBarriers)
{
    SmallVector<::vk::ImageMemoryBarrier> vkImageBarriers;
    SmallVector<::vk::BufferMemoryBarrier> vkBufferBarriers;

    ::vk::AccessFlags srcAccessFlags = {};
    ::vk::AccessFlags dstAccessFlags = {};

    for (const auto& bufferBarrier : bufferBarriers)
    {
        const BufferBarrier* pTrans = &bufferBarrier;
        Buffer* pBuffer = pTrans->pBuffer;

        if (ResourceState::UnorderedAccess == pTrans->currentState &&
            ResourceState::UnorderedAccess == pTrans->newState)
        {
            vkBufferBarriers.emplace_back()
                .setSrcAccessMask(::vk::AccessFlagBits::eShaderWrite)
                .setDstAccessMask(::vk::AccessFlagBits::eShaderWrite | ::vk::AccessFlagBits::eShaderRead);
        }
        else
        {
            vkBufferBarriers.emplace_back()
                .setSrcAccessMask(utils::getAccessFlags(pTrans->currentState))
                .setDstAccessMask(utils::getAccessFlags(pTrans->newState));
        }

        {
            auto& vkBufferBarrier = vkBufferBarriers.back();
            vkBufferBarrier.setBuffer(pBuffer->getHandle()).setSize(::vk::WholeSize).setOffset(0);

            if (pTrans->acquire)
            {
                vkBufferBarrier.srcQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
                vkBufferBarrier.dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if (pTrans->release)
            {
                vkBufferBarrier.srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                vkBufferBarrier.dstQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                vkBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                vkBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            srcAccessFlags |= vkBufferBarrier.srcAccessMask;
            dstAccessFlags |= vkBufferBarrier.dstAccessMask;

            pBuffer->m_resourceState = pTrans->newState;
        }
    }

    for (const auto& imageBarrier : imageBarriers)
    {
        const ImageBarrier* pTrans = &imageBarrier;
        Image* pImage = pTrans->pImage;

        if (ResourceState::UnorderedAccess == pTrans->currentState &&
            ResourceState::UnorderedAccess == pTrans->newState)
        {
            vkImageBarriers.emplace_back()
                .setSrcAccessMask(::vk::AccessFlagBits::eShaderWrite)
                .setDstAccessMask(::vk::AccessFlagBits::eShaderWrite | ::vk::AccessFlagBits::eShaderRead)
                .setOldLayout(::vk::ImageLayout::eGeneral)
                .setNewLayout(::vk::ImageLayout::eGeneral);
        }
        else
        {
            vkImageBarriers.emplace_back()
                .setSrcAccessMask(utils::getAccessFlags(pTrans->currentState))
                .setDstAccessMask(utils::getAccessFlags(pTrans->newState))
                .setOldLayout(utils::getImageLayout(pTrans->currentState))
                .setNewLayout(utils::getImageLayout(pTrans->newState));
        }

        {
            auto& vkImageBarrier = vkImageBarriers.back();
            vkImageBarrier.image = pImage->getHandle();
            vkImageBarrier.subresourceRange.aspectMask = utils::getImageAspect(pImage->getFormat());
            vkImageBarrier.subresourceRange.baseMipLevel = pTrans->subresourceBarrier ? pTrans->mipLevel : 0;
            vkImageBarrier.subresourceRange.levelCount = pTrans->subresourceBarrier ? 1 : ::vk::RemainingMipLevels;
            vkImageBarrier.subresourceRange.baseArrayLayer = pTrans->subresourceBarrier ? pTrans->arrayLayer : 0;
            vkImageBarrier.subresourceRange.layerCount = pTrans->subresourceBarrier ? 1 : ::vk::RemainingMipLevels;

            if (pTrans->acquire && pTrans->currentState != ResourceState::Undefined)
            {
                vkImageBarrier.srcQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
                vkImageBarrier.dstQueueFamilyIndex = m_pQueue->getFamilyIndex();
            }
            else if (pTrans->release && pTrans->currentState != ResourceState::Undefined)
            {
                vkImageBarrier.srcQueueFamilyIndex = m_pQueue->getFamilyIndex();
                vkImageBarrier.dstQueueFamilyIndex = m_pDevice->getQueue(pTrans->queueType)->getFamilyIndex();
            }
            else
            {
                vkImageBarrier.srcQueueFamilyIndex = ::vk::QueueFamilyIgnored;
                vkImageBarrier.dstQueueFamilyIndex = ::vk::QueueFamilyIgnored;
            }

            srcAccessFlags |= vkImageBarrier.srcAccessMask;
            dstAccessFlags |= vkImageBarrier.dstAccessMask;

            pImage->m_resourceState = pTrans->newState;
            pImage->m_layout = vkImageBarrier.newLayout;
        }
    }

    ::vk::PipelineStageFlags srcStageMask = m_pDevice->determinePipelineStageFlags(srcAccessFlags, m_pQueue->getType());
    ::vk::PipelineStageFlags dstStageMask = m_pDevice->determinePipelineStageFlags(dstAccessFlags, m_pQueue->getType());

    if (!vkBufferBarriers.empty() || !vkImageBarriers.empty())
    {
        getHandle().pipelineBarrier(srcStageMask, dstStageMask, {}, {}, vkBufferBarriers, vkImageBarriers);
    }
}
void CommandBuffer::transitionImageLayout(Image* pImage, ResourceState newState)
{
    aph::vk::ImageBarrier barrier{
        .pImage = pImage,
        .currentState = pImage->getResourceState(),
        .newState = newState,
        .subresourceBarrier = 1,
    };
    insertBarrier({ barrier });
}
void CommandBuffer::resetQueryPool(::vk::QueryPool pool, uint32_t first, uint32_t count)
{
    getHandle().resetQueryPool(pool, first, count);
}
void CommandBuffer::writeTimeStamp(::vk::PipelineStageFlagBits stage, ::vk::QueryPool pool, uint32_t queryIndex)
{
    getHandle().writeTimestamp(::vk::PipelineStageFlagBits::eBottomOfPipe, pool, queryIndex);
}
void CommandBuffer::update(Buffer* pBuffer, Range range, const void* data)
{
    getHandle().updateBuffer(pBuffer->getHandle(), range.offset, range.size, data);
}
void CommandBuffer::setResource(ArrayProxy<Sampler*> samplers, uint32_t set, uint32_t binding)
{
    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .samplers = { samplers.begin(), samplers.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}
void CommandBuffer::setResource(ArrayProxy<Image*> images, uint32_t set, uint32_t binding)
{
    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .images = { images.begin(), images.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}
void CommandBuffer::setResource(ArrayProxy<Buffer*> buffers, uint32_t set, uint32_t binding)
{
    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .buffers = { buffers.begin(), buffers.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}
void CommandBuffer::setResource(DescriptorUpdateInfo updateInfo, uint32_t set, uint32_t binding)
{
    auto& resBindings = m_commandState.resourceBindings;
    if (resBindings.bindings[set][binding] != updateInfo)
    {
        resBindings.bindings[set][binding] = std::move(updateInfo);
        resBindings.setBit.set(set);
        resBindings.setBindingBit[set].set(binding);
        resBindings.dirtyBinding[set].set(binding);
    }
}
void CommandBuffer::setProgram(ShaderProgram* pProgram)
{
    m_commandState.pProgram = pProgram;

    if (pProgram->getPipelineType() == PipelineType::Geometry)
    {
        setDirty(DirtyFlagBits::vertexInput);
    }

    if (auto setLayout = pProgram->getSetLayout(BindlessResource::ResourceSetIdx); setLayout->isBindless())
    {
        if (!m_commandState.bindlessResource)
        {
            m_commandState.bindlessResource = m_pDevice->getBindlessResource(pProgram);
            auto resourceSetLayout = m_commandState.bindlessResource->getResourceLayout();
            auto resourceSet = m_commandState.bindlessResource->getResourceSet();
            SmallVector<uint32_t> dynamicOffsets(resourceSetLayout->getDynamicUniformCount(), 0);
            getHandle().bindDescriptorSets(utils::VkCast(pProgram->getPipelineType()), pProgram->getPipelineLayout(),
                                           BindlessResource::ResourceSetIdx, { resourceSet->getHandle() },
                                           dynamicOffsets);
        }
    }
}
void CommandBuffer::setVertexInput(VertexInput inputInfo)
{
    m_commandState.graphics.vertexInput = std::move(inputInfo);
    setDirty(DirtyFlagBits::vertexInput);
}
void CommandBuffer::setDepthState(DepthState state)
{
    m_commandState.graphics.depthState = std::move(state);
    setDirty(DirtyFlagBits::dynamicState);
}

void CommandBuffer::flushDynamicGraphicsState()
{
    getHandle().setCullModeEXT(utils::VkCast(m_commandState.graphics.cullMode));
    // Set front face, cull mode is set in build_command_buffers.
    getHandle().setFrontFaceEXT(utils::VkCast(m_commandState.graphics.frontFace));

    bool wireframeEnabled = m_commandState.graphics.polygonMode == PolygonMode::Line;
    getHandle().setPolygonModeEXT(wireframeEnabled ? ::vk::PolygonMode::eLine : ::vk::PolygonMode::eFill);
    if (wireframeEnabled)
    {
        getHandle().setLineWidth(1.0f);
    }

    getHandle().setAlphaToOneEnableEXT(::vk::False);

    {
        ::vk::ColorBlendEquationEXT colorBlendEquationEXT{};
        getHandle().setColorBlendEquationEXT(0, { colorBlendEquationEXT });
    }

    getHandle().setRasterizerDiscardEnable(::vk::False);
    getHandle().setRasterizationSamplesEXT(utils::getSampleCountFlags(m_commandState.graphics.sampleCount));

    {
        // Use 1 sample per pixel
        const ::vk::SampleMask sample_mask = 0x1;
        getHandle().setSampleMaskEXT(::vk::SampleCountFlagBits::e1, &sample_mask);
    }

    // Do not use alpha to coverage or alpha to one because not using MSAA
    getHandle().setAlphaToCoverageEnableEXT(::vk::False);

    // Set depth state, the depth write. Don't enable depth bounds, bias, or stencil test.
    {
        auto& state = m_commandState.graphics.depthState;
        const ::vk::CompareOp op = utils::VkCast(state.compareOp);
        getHandle().setDepthWriteEnable(state.write ? ::vk::True : ::vk::False);
        getHandle().setDepthTestEnable(state.enable);
        getHandle().setDepthCompareOp(op);
        getHandle().setDepthClampEnableEXT(::vk::False);
        getHandle().setDepthBoundsTestEnableEXT(::vk::False);
        getHandle().setDepthBiasEnableEXT(::vk::False);
        getHandle().setStencilTestEnableEXT(::vk::False);
    }

    // Do not enable logic op
    getHandle().setLogicOpEnableEXT(::vk::False);

    {
        // Disable color blending
        ::vk::Bool32 color_blend_enables[] = { ::vk::False };
        getHandle().setColorBlendEnableEXT(0, color_blend_enables);
    }

    {
        // Use RGBA color write mask
        ::vk::ColorComponentFlags color_component_flags[] = { ::vk::ColorComponentFlagBits::eR |
                                                              ::vk::ColorComponentFlagBits::eG |
                                                              ::vk::ColorComponentFlagBits::eB |
                                                              ::vk::ColorComponentFlagBits::eA };
        getHandle().setColorWriteMaskEXT(0, 1, color_component_flags);
    }
}
void CommandBuffer::flushDescriptorSet(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset)
{
    auto& resBindings = m_commandState.resourceBindings;
    auto& pProgram = m_commandState.pProgram;

    if (m_commandState.bindlessResource)
    {
        m_commandState.bindlessResource->build();

        {
            const auto& bindless = m_commandState.bindlessResource;
            SmallVector<uint32_t> dynamicOffsets(bindless->getHandleLayout()->getDynamicUniformCount(), 0);
            ::vk::BindDescriptorSetsInfo bindDescriptorSetsInfo{};
            bindDescriptorSetsInfo.setFirstSet(BindlessResource::HandleSetIdx)
                .setLayout(pProgram->getPipelineLayout())
                .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                .setDynamicOffsets(dynamicOffset)
                .setDescriptorSets(bindless->getHandleSet()->getHandle())
                .setDynamicOffsets(dynamicOffsets);
            getHandle().bindDescriptorSets2(bindDescriptorSetsInfo);
        }
    }

    aph::utils::forEachBit(
        m_commandState.resourceBindings.setBit,
        [this, &resBindings, &pProgram](uint32_t setIdx)
        {
            if (m_commandState.bindlessResource && setIdx < BindlessResource::UpperBound)
            {
                return;
            }

            APH_ASSERT(setIdx < VULKAN_NUM_DESCRIPTOR_SETS);
            auto& set = resBindings.sets[setIdx];
            aph::utils::forEachBit(resBindings.setBindingBit[setIdx],
                                   [setIdx, &resBindings, &pProgram, &set](auto bindingIdx)
                                   {
                                       if (!resBindings.dirtyBinding[setIdx].test(bindingIdx))
                                       {
                                           CM_LOG_DEBUG("skip update");
                                           return;
                                       }

                                       if (set == nullptr)
                                       {
                                           auto setLayout = pProgram->getSetLayout(setIdx);
                                           set = setLayout->allocateSet();
                                       }
                                       APH_VR(set->update(resBindings.bindings[setIdx][bindingIdx]));
                                   });
            resBindings.dirtyBinding[setIdx].reset();

            SmallVector<uint32_t> dynamicOffsets(pProgram->getSetLayout(setIdx)->getDynamicUniformCount(), 0);
            getHandle().bindDescriptorSets(utils::VkCast(pProgram->getPipelineType()), pProgram->getPipelineLayout(),
                                           setIdx, { set->getHandle() }, dynamicOffsets);
        });

    if (m_commandState.dirty & DirtyFlagBits::pushConstant)
    {
        auto& range = m_commandState.pProgram->getPushConstantRange();
        getHandle().pushConstants(m_commandState.pProgram->getPipelineLayout(), range.stageFlags, 0,
                                  sizeof(m_commandState.resourceBindings.pushConstantData),
                                  m_commandState.resourceBindings.pushConstantData);
    }
}

void CommandBuffer::pushConstant(const void* pData, Range range)
{
    auto& resBinding = m_commandState.resourceBindings;
    APH_ASSERT(range.offset + range.size <= VULKAN_PUSH_CONSTANT_SIZE);
    std::memcpy(resBinding.pushConstantData + range.offset, pData, range.size);
    setDirty(DirtyFlagBits::pushConstant);
}

void CommandBuffer::setCullMode(const CullMode mode)
{
    m_commandState.graphics.cullMode = mode;
    setDirty(DirtyFlagBits::dynamicState);
}
void CommandBuffer::setFrontFaceWinding(const WindingMode mode)
{
    m_commandState.graphics.frontFace = mode;
    setDirty(DirtyFlagBits::dynamicState);
}
void CommandBuffer::setPolygonMode(const PolygonMode mode)
{
    m_commandState.graphics.polygonMode = mode;
    setDirty(DirtyFlagBits::dynamicState);
}
void CommandBuffer::setDirty(DirtyFlagBits dirtyFlagBits)
{
    m_commandState.dirty |= dirtyFlagBits;
}

void CommandBuffer::draw(DispatchArguments args, const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset)
{
    flushGraphicsCommand(dynamicOffset);
    getHandle().drawMeshTasksEXT(args.x, args.y, args.z);
}
} // namespace aph::vk
