#include "commandBuffer.h"
#include "bindless.h"
#include "device.h"
#include "exception/errorMacros.h"

namespace aph::vk
{

CommandBuffer::CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue, bool transient)
    : ResourceHandle(handle)
    , m_pDevice(pDevice)
    , m_pQueue(pQueue)
    , m_state(RecordState::Initial)
    , m_transient(transient)
    , m_breadcrumbs(pDevice->getCreateInfo().enableDebug,
                    std::format("Command Buffer ID: {:#x} (Queue type: {})",
                                reinterpret_cast<uintptr_t>(aph::vk::utils::GetCType(getHandle())),
                                utils::toString(pQueue->getType())))
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pDevice, "Device cannot be null");
    APH_ASSERT(pQueue, "Queue cannot be null");
    APH_ASSERT(handle != VK_NULL_HANDLE, "Command buffer handle cannot be null");
}

CommandBuffer::~CommandBuffer() = default;

auto CommandBuffer::generateBreadcrumbReport() const -> std::string
{
    if (!m_breadcrumbs.isEnabled() || m_state == RecordState::Initial)
    {
        return "No command buffer recording data available";
    }

    return m_breadcrumbs.toString("");
}

auto CommandBuffer::begin() -> Result
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_pDevice, "Device cannot be null");
    APH_ASSERT(getHandle() != VK_NULL_HANDLE, "Command buffer handle cannot be null");

    if (m_state == RecordState::Recording)
    {
        return { Result::RuntimeError, "Command buffer is not ready." };
    }

    // Begin command recording.
    ::vk::CommandBufferBeginInfo beginInfo;
    if (m_transient)
    {
        beginInfo.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    }

    auto result = getHandle().begin(beginInfo);
    if (result != ::vk::Result::eSuccess)
    {
        return utils::getResult(result);
    }

    // Mark CommandBuffer as recording and reset internal state.
    m_commandState = {};
    m_state        = RecordState::Recording;

    // Ensure breadcrumb tracking is enabled only in debug mode
    m_breadcrumbs.setEnabled(m_pDevice->getCreateInfo().enableDebug);

    // Reset breadcrumbs
    m_breadcrumbs.clear();
    m_currentScopeIndex = UINT32_MAX;

    return Result::Success;
}

auto CommandBuffer::end() -> Result
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(getHandle() != VK_NULL_HANDLE, "Command buffer handle cannot be null");

    if (m_state != RecordState::Recording)
    {
        return { Result::RuntimeError, "Commands are not recorded yet" };
    }

    m_state = RecordState::Executable;

    // Mark all pending breadcrumbs as completed
    m_breadcrumbs.completeAll();

    return utils::getResult(getHandle().end());
}

auto CommandBuffer::reset() -> Result
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_handle != VK_NULL_HANDLE, "Command buffer handle cannot be null");

    if (m_handle != VK_NULL_HANDLE)
    {
        getHandle().reset(::vk::CommandBufferResetFlagBits::eReleaseResources);
    }
    m_state = RecordState::Initial;

    // Reset breadcrumbs
    m_breadcrumbs.clear();
    m_currentScopeIndex = UINT32_MAX;

    return Result::Success;
}

void CommandBuffer::bindVertexBuffers(Buffer* pBuffer, uint32_t binding, std::size_t offset)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(pBuffer, "Vertex buffer cannot be null");
    APH_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS, "Binding index exceeds maximum allowed vertex buffers");

    auto& vertexState            = m_commandState.graphics.vertex;
    vertexState.buffers[binding] = pBuffer->getHandle();
    vertexState.offsets[binding] = offset;
    setDirty(DirtyFlagBits::vertexState);
    vertexState.dirty.set(binding);
}

void CommandBuffer::bindIndexBuffers(Buffer* pBuffer, std::size_t offset, IndexType indexType)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(pBuffer, "Index buffer cannot be null");
    APH_ASSERT(indexType != IndexType::NONE, "Index type must be specified");

    auto& indexState     = m_commandState.graphics.index;
    indexState.buffer    = pBuffer->getHandle();
    indexState.offset    = offset;
    indexState.indexType = indexType;
    setDirty(DirtyFlagBits::indexState);
}

void CommandBuffer::copy(Buffer* srcBuffer, Buffer* dstBuffer, Range range)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(srcBuffer, "Source buffer cannot be null");
    APH_ASSERT(dstBuffer, "Destination buffer cannot be null");
    APH_ASSERT(range.size > 0, "Copy size must be greater than 0");

    // Add breadcrumb for buffer copy operation
    std::string details =
        std::format("SrcBuffer={:#x}, DstBuffer={:#x}, Size={}", reinterpret_cast<uintptr_t>(srcBuffer),
                    reinterpret_cast<uintptr_t>(dstBuffer), range.size);
    uint32_t copyIndex = m_breadcrumbs.addBreadcrumb("BufferCopy", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::InProgress);

    ::vk::BufferCopy copyRegion{};
    copyRegion.setSize(range.size).setSrcOffset(0).setDstOffset(range.offset);
    getHandle().copyBuffer(srcBuffer->getHandle(), dstBuffer->getHandle(), { copyRegion });

    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::Completed);
}

void CommandBuffer::copy(Buffer* buffer, Image* image, ArrayProxy<BufferImageCopy> regions)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(buffer, "Buffer cannot be null");
    APH_ASSERT(image, "Image cannot be null");

    // Add breadcrumb for buffer to image copy operation
    std::string details = std::format("Buffer={:#x}, Image={:#x}, Regions={}", reinterpret_cast<uintptr_t>(buffer),
                                      reinterpret_cast<uintptr_t>(image), regions.size());
    uint32_t copyIndex  = m_breadcrumbs.addBreadcrumb("BufferToImageCopy", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::InProgress);

    if (regions.empty())
    {
        BufferImageCopy region{};

        region.imageSubresource.aspectMask = static_cast<uint32_t>(::vk::ImageAspectFlagBits::eColor);
        region.imageSubresource.layerCount = 1;
        region.imageExtent                 = { image->getWidth(), image->getHeight(), 1 };
        getHandle().copyBufferToImage(buffer->getHandle(), image->getHandle(), ::vk::ImageLayout::eTransferDstOptimal,
                                      { utils::VkCast(region) });
    }
    else
    {
        SmallVector<::vk::BufferImageCopy> vkRegions;
        vkRegions.reserve(regions.size());

        for (const auto& region : regions)
        {
            vkRegions.push_back(utils::VkCast(region));
        }

        getHandle().copyBufferToImage(buffer->getHandle(), image->getHandle(), ::vk::ImageLayout::eTransferDstOptimal,
                                      vkRegions);
    }

    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::Completed);
}

void CommandBuffer::copy(Image* srcImage, Image* dstImage, Extent3D extent, const ImageCopyInfo& srcCopyInfo,
                         const ImageCopyInfo& dstCopyInfo)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(srcImage && dstImage);

    // Add breadcrumb for image to image copy operation
    std::string details = std::format("SrcImage={:#x}, DstImage={:#x}", reinterpret_cast<uintptr_t>(srcImage),
                                      reinterpret_cast<uintptr_t>(dstImage));
    uint32_t copyIndex  = m_breadcrumbs.addBreadcrumb("ImageToImageCopy", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::InProgress);

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
        .setSrcOffset(utils::VkCast(srcCopyInfo.offset))
        .setSrcSubresource(utils::VkCast(srcCopyInfo.subResources))
        .setDstOffset(utils::VkCast(dstCopyInfo.offset))
        .setDstSubresource(utils::VkCast(dstCopyInfo.subResources));

    if (copyRegion.dstSubresource.aspectMask == ::vk::ImageAspectFlagBits::eNone)
    {
        copyRegion.dstSubresource.aspectMask = utils::getImageAspect(dstImage->getFormat());
    }
    if (copyRegion.srcSubresource.aspectMask == ::vk::ImageAspectFlagBits::eNone)
    {
        copyRegion.srcSubresource.aspectMask = utils::getImageAspect(srcImage->getFormat());
    }

    APH_ASSERT(copyRegion.srcSubresource.aspectMask == copyRegion.dstSubresource.aspectMask);

    copyRegion.extent.width  = srcImage->getWidth();
    copyRegion.extent.height = srcImage->getHeight();
    copyRegion.extent.depth  = 1;

    getHandle().copyImage(srcImage->getHandle(), ::vk::ImageLayout::eTransferSrcOptimal, dstImage->getHandle(),
                          ::vk::ImageLayout::eTransferDstOptimal, { copyRegion });

    m_breadcrumbs.updateBreadcrumb(copyIndex, BreadcrumbState::Completed);
}

void CommandBuffer::draw(DrawArguments args)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this draw call
    std::string details = std::format("VertexCount={}, InstanceCount={}", args.vertexCount, args.instanceCount);
    uint32_t drawIndex  = m_breadcrumbs.addBreadcrumb("Draw", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::InProgress);

    // Flush any pending state changes and perform the draw
    flushGraphicsCommand();
    getHandle().draw(args.vertexCount, args.instanceCount, args.firstVertex, args.firstInstance);

    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::blit(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo,
                         const ImageBlitInfo& dstBlitInfo, Filter filter) -> void
{
    APH_PROFILER_SCOPE();

    // Add breadcrumb for blit operation
    std::string details =
        std::format("SrcImage={:#x}, DstImage={:#x}, Filter={}", reinterpret_cast<uintptr_t>(srcImage),
                    reinterpret_cast<uintptr_t>(dstImage), static_cast<int>(filter));
    uint32_t blitIndex = m_breadcrumbs.addBreadcrumb("ImageBlit", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(blitIndex, BreadcrumbState::InProgress);

    const auto addOffset = [](const Offset3D& a, const Offset3D& b) -> Offset3D
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    };

    const auto isExtentValid = [](const Offset3D& extent) -> bool
    {
        return extent.x != 0 || extent.y != 0 || extent.z != 0;
    };

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

    vkBlitInfo.srcOffsets[0] = utils::VkCast(srcBlitInfo.offset);

    if (isExtentValid(srcBlitInfo.extent))
    {
        vkBlitInfo.srcOffsets[1] = utils::VkCast(addOffset(srcBlitInfo.offset, srcBlitInfo.extent));
    }
    else
    {
        vkBlitInfo.srcOffsets[1] = ::vk::Offset3D{ static_cast<int32_t>(srcImage->getWidth()),
                                                   static_cast<int32_t>(srcImage->getHeight()), 1 };
    }

    vkBlitInfo.dstOffsets[0] = utils::VkCast(dstBlitInfo.offset);
    if (isExtentValid(dstBlitInfo.extent))
    {
        vkBlitInfo.dstOffsets[1] = utils::VkCast(addOffset(dstBlitInfo.offset, dstBlitInfo.extent));
    }
    else
    {
        vkBlitInfo.dstOffsets[1] = ::vk::Offset3D{ static_cast<int32_t>(dstImage->getWidth()),
                                                   static_cast<int32_t>(dstImage->getHeight()), 1 };
    }

    // Instead of accessing m_layout directly, use appropriate layouts based on usage
    ::vk::ImageLayout srcLayout = ::vk::ImageLayout::eTransferSrcOptimal;
    ::vk::ImageLayout dstLayout = ::vk::ImageLayout::eTransferDstOptimal;

    getHandle().blitImage(srcImage->getHandle(), srcLayout, dstImage->getHandle(), dstLayout, { vkBlitInfo },
                          utils::VkCast(filter));

    m_breadcrumbs.updateBreadcrumb(blitIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::endRendering() -> void
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add breadcrumb for end rendering
    uint32_t renderingIndex = m_breadcrumbs.addBreadcrumb("EndRendering", "", m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(renderingIndex, BreadcrumbState::InProgress);

    getHandle().endRendering();

    m_breadcrumbs.updateBreadcrumb(renderingIndex, BreadcrumbState::Completed);
}

void CommandBuffer::dispatch(DispatchArguments args)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this dispatch call
    std::string details    = std::format("ThreadGroups={}x{}x{}", args.x, args.y, args.z);
    uint32_t dispatchIndex = m_breadcrumbs.addBreadcrumb("Dispatch", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(dispatchIndex, BreadcrumbState::InProgress);

    flushComputeCommand();
    getHandle().dispatch(args.x, args.y, args.z);

    m_breadcrumbs.updateBreadcrumb(dispatchIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::dispatch(Buffer* pBuffer, std::size_t offset) -> void
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this indirect dispatch call
    std::string details    = std::format("BufferOffset={}", offset);
    uint32_t dispatchIndex = m_breadcrumbs.addBreadcrumb("DispatchIndirect", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(dispatchIndex, BreadcrumbState::InProgress);

    flushComputeCommand();
    getHandle().dispatchIndirect(pBuffer->getHandle(), offset);

    m_breadcrumbs.updateBreadcrumb(dispatchIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::draw(Buffer* pBuffer, std::size_t offset, uint32_t drawCount, uint32_t stride) -> void
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this indirect draw call
    std::string details = std::format("DrawCount={}, BufferOffset={}", drawCount, offset);
    uint32_t drawIndex  = m_breadcrumbs.addBreadcrumb("DrawIndirect", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::InProgress);

    flushGraphicsCommand();
    getHandle().drawIndirect(pBuffer->getHandle(), offset, drawCount, stride);

    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::Completed);
}

void CommandBuffer::drawIndexed(DrawIndexArguments args)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this indexed draw call
    std::string details = std::format("IndexCount={}, InstanceCount={}", args.indexCount, args.instanceCount);
    uint32_t drawIndex  = m_breadcrumbs.addBreadcrumb("DrawIndexed", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::InProgress);

    flushGraphicsCommand();
    getHandle().drawIndexed(args.indexCount, args.instanceCount, args.firstIndex, args.vertexOffset,
                            args.firstInstance);

    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::beginRendering(const RenderingInfo& renderingInfo) -> void
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(!renderingInfo.colors.empty() || renderingInfo.depth.image,
               "At least one color attachment or depth attachment must be provided");

    // Add breadcrumb for begin rendering
    std::string details     = std::format("ColorAttachments={}, DepthAttachment={}", renderingInfo.colors.size(),
                                      renderingInfo.depth.image ? "true" : "false");
    uint32_t renderingIndex = m_breadcrumbs.addBreadcrumb("BeginRendering", details, m_currentScopeIndex);
    m_breadcrumbs.updateBreadcrumb(renderingIndex, BreadcrumbState::InProgress);

    m_commandState.graphics.color = renderingInfo.colors;
    m_commandState.graphics.depth = renderingInfo.depth;

    APH_ASSERT(!m_commandState.graphics.color.empty() || m_commandState.graphics.depth.image);

    // Validate color attachments
    for (const auto& color : m_commandState.graphics.color)
    {
        APH_ASSERT(color.image, "Color attachment image cannot be null");
        APH_ASSERT(color.image->getView(), "Color attachment image view cannot be null");
        APH_ASSERT(color.image->getWidth() > 0 && color.image->getHeight() > 0,
                   "Color attachment dimensions must be greater than 0");
    }

    // Validate depth attachment if present
    if (m_commandState.graphics.depth.image)
    {
        APH_ASSERT(m_commandState.graphics.depth.image->getView(), "Depth attachment image view cannot be null");
    }

    SmallVector<::vk::RenderingAttachmentInfo> vkColors;
    SmallVector<::vk::Viewport> vkViewports;
    SmallVector<::vk::Rect2D> vkScissors;
    vkColors.reserve(m_commandState.graphics.color.size());
    for (const auto& color : m_commandState.graphics.color)
    {
        auto& image = color.image;
        ::vk::RenderingAttachmentInfo vkColorAttrInfo{};
        vkColorAttrInfo.setImageView(image->getView()->getHandle())
            .setImageLayout(utils::VkCast(color.layout.value_or(ImageLayout::ColorAttachmentOptimal)))
            .setLoadOp(utils::VkCast(color.loadOp.value_or(AttachmentLoadOp::Clear)))
            .setStoreOp(utils::VkCast(color.storeOp.value_or(AttachmentStoreOp::Store)))
            .setClearValue(color.clear.has_value() ? utils::VkCast(color.clear.value()) :
                                                     ::vk::ClearValue{}.setColor({ 0.0f, 0.0f, 0.0f, 1.0f }));

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
    if (renderingInfo.renderArea.has_value())
    {
        vkRenderingInfo.setRenderArea(utils::VkCast(renderingInfo.renderArea.value()));
    }
    else
    {
        APH_ASSERT(!vkScissors.empty(), "No scissor rects available for default render area");
        vkRenderingInfo.setRenderArea(vkScissors[0]);
    }
    vkRenderingInfo.setLayerCount(1).setColorAttachments(vkColors);

    ::vk::RenderingAttachmentInfo vkDepth;
    if (const auto& depth = m_commandState.graphics.depth; depth.image != nullptr)
    {
        vkDepth.setImageView(depth.image->getView()->getHandle())
            .setImageLayout(utils::VkCast(depth.layout.value_or(ImageLayout::DepthAttachmentOptimal)))
            .setLoadOp(utils::VkCast(depth.loadOp.value_or(AttachmentLoadOp::Clear)))
            .setStoreOp(utils::VkCast(depth.storeOp.value_or(AttachmentStoreOp::DontCare)))
            .setClearValue(depth.clear.has_value() ? utils::VkCast(depth.clear.value()) :
                                                     ::vk::ClearValue{}.setDepthStencil({ 1.0f, 0x00 }));

        vkRenderingInfo.setPDepthAttachment(&vkDepth);
    }
    getHandle().beginRendering(vkRenderingInfo);
}

auto CommandBuffer::flushComputeCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset) -> void
{
    APH_PROFILER_SCOPE();

    auto pProgram = m_commandState.pProgram;
    APH_ASSERT(pProgram, "No shader program bound");
    APH_ASSERT(pProgram->getPipelineType() == PipelineType::Compute, "Program must be compute shader");
    APH_ASSERT(pProgram->getShaderObject(ShaderStage::CS) != VK_NULL_HANDLE, "Compute shader object is null");

    SmallVector<::vk::ShaderStageFlagBits> stages = { ::vk::ShaderStageFlagBits::eCompute };
    SmallVector<::vk::ShaderEXT> shaderObjs       = { pProgram->getShaderObject(ShaderStage::CS) };
    getHandle().bindShadersEXT(stages, shaderObjs);
    flushDescriptorSet(dynamicOffset);
    m_commandState.dirty = {};
}

auto CommandBuffer::flushGraphicsCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset) -> void
{
    APH_PROFILER_SCOPE();

    auto pProgram = m_commandState.pProgram;
    APH_ASSERT(pProgram, "No shader program bound");
    APH_ASSERT(pProgram->getPipelineType() == PipelineType::Geometry ||
                   pProgram->getPipelineType() == PipelineType::Mesh,
               "Program must be graphics shader");

    flushDynamicGraphicsState();

    // shader object binding
    {
        const auto& vertexInput = m_commandState.graphics.vertexInput;
        auto& vertexState       = m_commandState.graphics.vertex;
        auto& indexState        = m_commandState.graphics.index;
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
            APH_ASSERT(pProgram->getShaderObject(ShaderStage::VS) != VK_NULL_HANDLE, "Vertex shader object is null");
            APH_ASSERT(pProgram->getShaderObject(ShaderStage::FS) != VK_NULL_HANDLE, "Fragment shader object is null");

            shaderObjs[VS] = pProgram->getShaderObject(ShaderStage::VS);
            shaderObjs[FS] = pProgram->getShaderObject(ShaderStage::FS);

            SmallVector<::vk::VertexInputBindingDescription2EXT> vkBindings;
            SmallVector<::vk::VertexInputAttributeDescription2EXT> vkAttributes;

            if (m_commandState.dirty & DirtyFlagBits::vertexInput)
            {
                const VertexInput& vstate = vertexInput.value_or(pProgram->getVertexInput());

                APH_ASSERT(!vstate.attributes.empty(), "Vertex input has no attributes");
                APH_ASSERT(vstate.bindings.size() > 0, "Vertex input has no bindings");

                vkAttributes.resize(vstate.attributes.size());
                SmallVector<bool> bufferAlreadyBound(vstate.bindings.size());

                for (uint32_t i = 0; i != vkAttributes.size(); i++)
                {
                    const auto& attr = vstate.attributes[i];
                    APH_ASSERT(attr.binding < vstate.bindings.size(), "Attribute references invalid binding");
                    APH_ASSERT(attr.format != Format::Undefined, "Attribute format is undefined");

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
                for (auto [binding, bindingCount] : aph::utils::forEachBitRange(vertexState.dirty))
                {
                    getHandle().bindVertexBuffers(binding, bindingCount, vertexState.buffers + binding,
                                                  vertexState.offsets + binding);
                }
                vertexState.dirty.reset();
            }

            if (m_commandState.dirty & DirtyFlagBits::indexState)
            {
                getHandle().bindIndexBuffer(indexState.buffer, indexState.offset, utils::VkCast(indexState.indexType));
            }
        }
        else if (pProgram->getPipelineType() == PipelineType::Mesh)
        {
            if (pProgram->getShaderObject(ShaderStage::TS) != VK_NULL_HANDLE)
            {
                shaderObjs[TS] = pProgram->getShaderObject(ShaderStage::TS);
            }

            APH_ASSERT(pProgram->getShaderObject(ShaderStage::MS) != VK_NULL_HANDLE, "Mesh shader object is null");
            APH_ASSERT(pProgram->getShaderObject(ShaderStage::FS) != VK_NULL_HANDLE, "Fragment shader object is null");

            shaderObjs[MS] = pProgram->getShaderObject(ShaderStage::MS);
            shaderObjs[FS] = pProgram->getShaderObject(ShaderStage::FS);
        }
        else
        {
            APH_ASSERT(false, "Invalid pipeline type");
            CM_LOG_ERR("Invalid pipeline type.");
        }

        getHandle().bindShadersEXT(stages, shaderObjs);
    }

    flushDescriptorSet(dynamicOffset);
    m_commandState.dirty = {};
}

void CommandBuffer::beginDebugLabel(const DebugLabel& label)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a new breadcrumb for this debug label scope
    uint32_t parentIndex = m_currentScopeIndex;
    m_currentScopeIndex  = m_breadcrumbs.addBreadcrumb(label.name, "", parentIndex);

    // Add the label to the command buffer
#ifdef APH_DEBUG
    auto vkLabel = utils::VkCast(label);
    getHandle().beginDebugUtilsLabelEXT(vkLabel);
#endif
}

void CommandBuffer::endDebugLabel()
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Mark the current breadcrumb scope as completed
    if (m_currentScopeIndex != UINT32_MAX)
    {
        m_breadcrumbs.updateBreadcrumb(m_currentScopeIndex, BreadcrumbState::Completed);

        // Find the parent scope
        for (const auto& crumb : m_breadcrumbs.getBreadcrumbs())
        {
            if (crumb.index == m_currentScopeIndex)
            {
                m_currentScopeIndex =
                    (crumb.depth > 0) ? m_breadcrumbs.findParentIndex(m_currentScopeIndex) : UINT32_MAX;
                break;
            }
        }
    }

    // End the debug label
#ifdef APH_DEBUG
    getHandle().endDebugUtilsLabelEXT();
#endif
}

void CommandBuffer::insertDebugLabel(const DebugLabel& label)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this debug label (as a leaf node)
    uint32_t labelIndex = m_breadcrumbs.addBreadcrumb(label.name, "", m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(labelIndex, BreadcrumbState::Completed);

    // Insert the debug label in the command buffer
#ifdef APH_DEBUG
    auto vkLabel = utils::VkCast(label);
    getHandle().insertDebugUtilsLabelEXT(vkLabel);
#endif
}

void CommandBuffer::insertBarrier(ArrayProxy<ImageBarrier> pImageBarriers)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this barrier
    std::string details = std::format("Images={}", pImageBarriers.size());
    uint32_t barrierIndex =
        m_breadcrumbs.addBreadcrumb("----------Barrier----------", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(barrierIndex, BreadcrumbState::InProgress);

    // Call the main implementation
    insertBarrier({}, pImageBarriers, barrierIndex);
}

void CommandBuffer::insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this barrier
    std::string details = std::format("Buffers={}", pBufferBarriers.size());
    uint32_t barrierIndex =
        m_breadcrumbs.addBreadcrumb("----------Barrier----------", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(barrierIndex, BreadcrumbState::InProgress);

    // Call the main implementation
    insertBarrier(pBufferBarriers, {}, barrierIndex);
}

void CommandBuffer::insertBarrier(ArrayProxy<BufferBarrier> bufferBarriers, ArrayProxy<ImageBarrier> imageBarriers)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");

    // Add a breadcrumb for this barrier
    std::string details = std::format("Buffers={}, Images={}", bufferBarriers.size(), imageBarriers.size());
    uint32_t barrierIndex =
        m_breadcrumbs.addBreadcrumb("----------Barrier----------", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(barrierIndex, BreadcrumbState::InProgress);

    // Call the implementation
    insertBarrier(bufferBarriers, imageBarriers, barrierIndex);
}

// Private implementation that takes an existing breadcrumb index
void CommandBuffer::insertBarrier(ArrayProxy<BufferBarrier> bufferBarriers, ArrayProxy<ImageBarrier> imageBarriers,
                                  uint32_t barrierIndex)
{
    // Validate buffer barriers
    for (const auto& barrier : bufferBarriers)
    {
        APH_ASSERT(barrier.pBuffer, "Buffer in barrier cannot be null");
    }

    // Validate image barriers
    for (const auto& barrier : imageBarriers)
    {
        APH_ASSERT(barrier.pImage, "Image in barrier cannot be null");
    }

    SmallVector<::vk::ImageMemoryBarrier> vkImageBarriers;
    SmallVector<::vk::BufferMemoryBarrier> vkBufferBarriers;

    ::vk::AccessFlags srcAccessFlags = {};
    ::vk::AccessFlags dstAccessFlags = {};

    for (const auto& bufferBarrier : bufferBarriers)
    {
        const BufferBarrier* pTrans = &bufferBarrier;
        Buffer* pBuffer             = pTrans->pBuffer;

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
        }
    }

    for (const auto& imageBarrier : imageBarriers)
    {
        const ImageBarrier* pTrans = &imageBarrier;
        Image* pImage              = pTrans->pImage;

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
            auto& vkImageBarrier                           = vkImageBarriers.back();
            vkImageBarrier.image                           = pImage->getHandle();
            vkImageBarrier.subresourceRange.aspectMask     = utils::getImageAspect(pImage->getFormat());
            vkImageBarrier.subresourceRange.baseMipLevel   = pTrans->subresourceBarrier ? pTrans->mipLevel : 0;
            vkImageBarrier.subresourceRange.levelCount     = pTrans->subresourceBarrier ? 1 : ::vk::RemainingMipLevels;
            vkImageBarrier.subresourceRange.baseArrayLayer = pTrans->subresourceBarrier ? pTrans->arrayLayer : 0;
            vkImageBarrier.subresourceRange.layerCount     = pTrans->subresourceBarrier ? 1 : ::vk::RemainingMipLevels;

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
        }
    }

    ::vk::PipelineStageFlags srcStageMask = m_pDevice->determinePipelineStageFlags(srcAccessFlags, m_pQueue->getType());
    ::vk::PipelineStageFlags dstStageMask = m_pDevice->determinePipelineStageFlags(dstAccessFlags, m_pQueue->getType());

    if (!vkBufferBarriers.empty() || !vkImageBarriers.empty())
    {
        getHandle().pipelineBarrier(srcStageMask, dstStageMask, {}, {}, vkBufferBarriers, vkImageBarriers);
    }

    // Update breadcrumb state
    if (barrierIndex != UINT32_MAX)
    {
        m_breadcrumbs.updateBreadcrumb(barrierIndex, BreadcrumbState::Completed);
    }
}

auto CommandBuffer::transitionImageLayout(Image* pImage, ResourceState newState) -> void
{
    APH_PROFILER_SCOPE();

    std::string details =
        std::format("Image={:#x}, NewState={}", reinterpret_cast<uintptr_t>(pImage), static_cast<int>(newState));
    uint32_t transitionIndex = m_breadcrumbs.addBreadcrumb("TransitionImageLayout", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(transitionIndex, BreadcrumbState::InProgress);

    aph::vk::ImageBarrier barrier{
        .pImage             = pImage,
        .currentState       = ResourceState::Undefined, // Default to undefined, should be provided by caller
        .newState           = newState,
        .subresourceBarrier = 1,
    };
    insertBarrier({ barrier });

    m_breadcrumbs.updateBreadcrumb(transitionIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::transitionImageLayout(Image* pImage, ResourceState currentState, ResourceState newState) -> void
{
    APH_PROFILER_SCOPE();

    std::string details = std::format("Image={:#x}, CurrentState={}, NewState={}", reinterpret_cast<uintptr_t>(pImage),
                                      static_cast<int>(currentState), static_cast<int>(newState));
    uint32_t transitionIndex = m_breadcrumbs.addBreadcrumb("TransitionImageLayout", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(transitionIndex, BreadcrumbState::InProgress);

    aph::vk::ImageBarrier barrier{
        .pImage             = pImage,
        .currentState       = currentState,
        .newState           = newState,
        .subresourceBarrier = 1,
    };
    insertBarrier({ barrier });

    m_breadcrumbs.updateBreadcrumb(transitionIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::resetQueryPool(QueryPool* pQueryPool, uint32_t first, uint32_t count) -> void
{
    APH_PROFILER_SCOPE();

    APH_ASSERT(pQueryPool != nullptr, "Query pool cannot be null");

    getHandle().resetQueryPool(pQueryPool->getHandle(), first, count);
}

auto CommandBuffer::writeTimeStamp(PipelineStage stage, QueryPool* pQueryPool, uint32_t queryIndex) -> void
{
    APH_PROFILER_SCOPE();

    APH_ASSERT(pQueryPool != nullptr, "Query pool cannot be null");
    APH_ASSERT(pQueryPool->getQueryType() == QueryType::Timestamp, "Query pool must be of timestamp type");

    getHandle().writeTimestamp(utils::VkCast(stage), pQueryPool->getHandle(), queryIndex);
}

auto CommandBuffer::update(Buffer* pBuffer, Range range, const void* data) -> void
{
    APH_PROFILER_SCOPE();

    std::string details =
        std::format("Buffer={:#x}, Offset={}, Size={}", reinterpret_cast<uintptr_t>(pBuffer), range.offset, range.size);
    uint32_t updateIndex = m_breadcrumbs.addBreadcrumb("UpdateBuffer", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(updateIndex, BreadcrumbState::InProgress);

    getHandle().updateBuffer(pBuffer->getHandle(), range.offset, range.size, data);

    m_breadcrumbs.updateBreadcrumb(updateIndex, BreadcrumbState::Completed);
}

auto CommandBuffer::setResource(ArrayProxy<Sampler*> samplers, uint32_t set, uint32_t binding) -> void
{
    APH_PROFILER_SCOPE();
    DescriptorUpdateInfo newUpdate = {
        .binding  = binding,
        .samplers = { samplers.begin(), samplers.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}

auto CommandBuffer::setResource(ArrayProxy<Image*> images, uint32_t set, uint32_t binding) -> void
{
    APH_PROFILER_SCOPE();
    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .images  = { images.begin(), images.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}

auto CommandBuffer::setResource(ArrayProxy<Buffer*> buffers, uint32_t set, uint32_t binding) -> void
{
    APH_PROFILER_SCOPE();
    DescriptorUpdateInfo newUpdate = {
        .binding = binding,
        .buffers = { buffers.begin(), buffers.end() },
    };
    setResource(std::move(newUpdate), set, binding);
}

auto CommandBuffer::setResource(DescriptorUpdateInfo updateInfo, uint32_t set, uint32_t binding) -> void
{
    APH_PROFILER_SCOPE();
    auto& resBindings = m_commandState.resourceBindings;
    if (resBindings.bindings[set][binding] != updateInfo)
    {
        resBindings.bindings[set][binding] = std::move(updateInfo);
        resBindings.setBit.set(set);
        resBindings.setBindingBit[set].set(binding);
        resBindings.dirtyBinding[set].set(binding);
    }
}

auto CommandBuffer::setProgram(ShaderProgram* pProgram) -> void
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_state == RecordState::Recording, "Command buffer must be in recording state");
    APH_ASSERT(pProgram, "Shader program cannot be null");

    m_commandState.pProgram = pProgram;

    if (pProgram->getPipelineType() == PipelineType::Geometry)
    {
        setDirty(DirtyFlagBits::vertexInput);
    }
}

auto CommandBuffer::setVertexInput(VertexInput inputInfo) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.graphics.vertexInput = std::move(inputInfo);
    setDirty(DirtyFlagBits::vertexInput);
}

auto CommandBuffer::setDepthState(DepthState state) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.graphics.depthState = std::move(state);
    setDirty(DirtyFlagBits::dynamicState);
}

auto CommandBuffer::flushDynamicGraphicsState() -> void
{
    APH_PROFILER_SCOPE();
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
        auto& state              = m_commandState.graphics.depthState;
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

auto CommandBuffer::flushDescriptorSet(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset) -> void
{
    APH_PROFILER_SCOPE();
    auto& resBindings = m_commandState.resourceBindings;
    auto& pProgram    = m_commandState.pProgram;

    if (pProgram->getSetLayout(BindlessResource::eResourceSetIdx)->isBindless())
    {
        if (!m_commandState.bindlessResource)
        {
            m_commandState.bindlessResource = m_pDevice->getBindlessResource();
            auto resourceSetLayout          = m_commandState.bindlessResource->getResourceLayout();
            auto resourceSet                = m_commandState.bindlessResource->getResourceSet();
            SmallVector<uint32_t> dynamicOffsets(resourceSetLayout->getDynamicUniformCount(), 0);
            ::vk::BindDescriptorSetsInfo bindDescriptorSetsInfo{};
            bindDescriptorSetsInfo.setFirstSet(BindlessResource::eResourceSetIdx)
                .setLayout(m_commandState.bindlessResource->getPipelineLayout()->getHandle())
                .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                .setDynamicOffsets(dynamicOffsets)
                .setDescriptorSets(resourceSet->getHandle());
            getHandle().bindDescriptorSets2(bindDescriptorSetsInfo);
        }

        m_commandState.bindlessResource->build();

        {
            const auto& bindless = m_commandState.bindlessResource;
            ::vk::BindDescriptorSetsInfo bindDescriptorSetsInfo{};
            bindDescriptorSetsInfo.setFirstSet(BindlessResource::eHandleSetIdx)
                .setLayout(bindless->getPipelineLayout()->getHandle())
                .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                .setDynamicOffsets(dynamicOffset)
                .setDescriptorSets(bindless->getHandleSet()->getHandle());

            SmallVector<uint32_t> dOffset(bindless->getHandleLayout()->getDynamicUniformCount(), 0);
            if (dynamicOffset.empty())
            {
                bindDescriptorSetsInfo.setDynamicOffsets(dOffset);
            }
            getHandle().bindDescriptorSets2(bindDescriptorSetsInfo);
        }
    }
    else
    {
        for (uint32_t setIdx : aph::utils::forEachBit(m_commandState.resourceBindings.setBit))
        {
            if (m_commandState.bindlessResource && setIdx < BindlessResource::eUpperBound)
            {
                return;
            }

            APH_ASSERT(setIdx < VULKAN_NUM_DESCRIPTOR_SETS);
            auto& set = resBindings.sets[setIdx];

            for (uint32_t bindingIdx : aph::utils::forEachBit(resBindings.setBindingBit[setIdx]))
            {
                if (!resBindings.dirtyBinding[setIdx].test(bindingIdx))
                {
                    CM_LOG_DEBUG("skip update");
                    return;
                }

                if (set == nullptr)
                {
                    auto setLayout = pProgram->getSetLayout(setIdx);
                    set            = setLayout->allocateSet();
                }
                APH_VERIFY_RESULT(set->update(resBindings.bindings[setIdx][bindingIdx]));
            }

            resBindings.dirtyBinding[setIdx].reset();

            SmallVector<uint32_t> dynamicOffsets(pProgram->getSetLayout(setIdx)->getDynamicUniformCount(), 0);
            ::vk::BindDescriptorSetsInfo bindDescriptorSetsInfo{};
            bindDescriptorSetsInfo.setFirstSet(setIdx)
                .setLayout(pProgram->getPipelineLayout()->getHandle())
                .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                .setDynamicOffsets(dynamicOffsets)
                .setDescriptorSets(set->getHandle());
            getHandle().bindDescriptorSets2(bindDescriptorSetsInfo);
        }
    }

    if (m_commandState.dirty & DirtyFlagBits::pushConstant)
    {
        ::vk::PushConstantRange range = utils::VkCast(m_commandState.pProgram->getPushConstantRange());
        getHandle().pushConstants(m_commandState.pProgram->getPipelineLayout()->getHandle(), range.stageFlags, 0,
                                  sizeof(m_commandState.resourceBindings.pushConstantData),
                                  m_commandState.resourceBindings.pushConstantData);
    }
}

auto CommandBuffer::pushConstant(const void* pData, Range range) -> void
{
    APH_PROFILER_SCOPE();
    auto& resBinding = m_commandState.resourceBindings;
    APH_ASSERT(range.offset + range.size <= VULKAN_PUSH_CONSTANT_SIZE);
    std::memcpy(resBinding.pushConstantData + range.offset, pData, range.size);
    setDirty(DirtyFlagBits::pushConstant);
}

auto CommandBuffer::setCullMode(CullMode mode) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.graphics.cullMode = mode;
    setDirty(DirtyFlagBits::dynamicState);
}

auto CommandBuffer::setFrontFaceWinding(WindingMode mode) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.graphics.frontFace = mode;
    setDirty(DirtyFlagBits::dynamicState);
}

auto CommandBuffer::setPolygonMode(PolygonMode mode) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.graphics.polygonMode = mode;
    setDirty(DirtyFlagBits::dynamicState);
}

auto CommandBuffer::setDirty(DirtyFlagBits dirtyFlagBits) -> void
{
    APH_PROFILER_SCOPE();
    m_commandState.dirty |= dirtyFlagBits;
}

auto CommandBuffer::draw(DispatchArguments args, const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset) -> void
{
    APH_PROFILER_SCOPE();

    // Add a breadcrumb for this mesh task draw call
    std::string details = std::format("TaskGroups={}x{}x{}", args.x, args.y, args.z);
    uint32_t drawIndex  = m_breadcrumbs.addBreadcrumb("DrawMeshTasks", details, m_currentScopeIndex, true);
    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::InProgress);

    flushGraphicsCommand(dynamicOffset);
    getHandle().drawMeshTasksEXT(args.x, args.y, args.z);

    m_breadcrumbs.updateBreadcrumb(drawIndex, BreadcrumbState::Completed);
}
} // namespace aph::vk
