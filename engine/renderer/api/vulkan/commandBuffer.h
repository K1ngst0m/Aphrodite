#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "device.h"
#include "renderer/gpuResource.h"

namespace vkl {
class VulkanCommandBuffer : public ResourceHandle<VkCommandBuffer> {
public:
    VulkanCommandBuffer(VulkanCommandPool *pool, VkCommandBuffer handle);

    ~VulkanCommandBuffer();

    VulkanCommandPool *GetPool() {
        return m_pool;
    }

    VkResult begin(VkCommandBufferUsageFlags flags);
    VkResult end();
    VkResult reset();

    void cmdBeginRenderPass(const VkRenderPassBeginInfo *pBeginInfo);
    void cmdNextSubpass();
    void cmdEndRenderPass();

    // void CmdBindPipeline(VkPipeline pipeline);
    // void CmdPushConstants(uint32_t offset, uint32_t size, const void *pValues);
    // void CmdBindBuffer(Buffer *pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
    // void CmdBindBufferView(BufferView *pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
    // void CmdBindImageView(ImageView *pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
    // void CmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
    // void CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, Buffer **ppBuffers, const VkDeviceSize *pOffsets);
    // void CmdBindIndexBuffer(Buffer *pBuffer, VkDeviceSize offset, VkIndexType indexType);
    // void CmdSetVertexInputFormat(VertexInputFormat *pFormat);
    // void CmdSetViewportState(uint32_t viewportCount);
    // void CmdSetInputAssemblyState(const VezInputAssemblyState *pStateInfo);
    // void CmdSetRasterizationState(const VezRasterizationState *pStateInfo);
    // void CmdSetMultisampleState(const VezMultisampleState *pStateInfo);
    // void CmdSetDepthStencilState(const VezDepthStencilState *pStateInfo);
    // void CmdSetColorBlendState(const VezColorBlendState *pStateInfo);
    // void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports);
    // void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors);
    // void CmdSetLineWidth(float lineWidth);
    // void CmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
    // void CmdSetBlendConstants(const float blendConstants[4]);
    // void CmdSetDepthBounds(float minDepthBounds, float maxDepthBounds);
    // void CmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
    // void CmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
    // void CmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
    // void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    // void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    // void CmdDrawIndirect(Buffer *pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    // void CmdDrawIndexedIndirect(Buffer *pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    // void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    // void CmdDispatchIndirect(Buffer *pBuffer, VkDeviceSize offset);
    // void CmdCopyBuffer(Buffer *pSrcBuffer, Buffer *pDstBuffer, uint32_t regionCount, const VezBufferCopy *pRegions);
    // void CmdCopyImage(Image *pSrcImage, Image *pDstImage, uint32_t regionCount, const VezImageCopy *pRegions);
    // void CmdBlitImage(Image *pSrcImage, Image *pDstImage, uint32_t regionCount, const VezImageBlit *pRegions, VkFilter filter);
    // void CmdCopyBufferToImage(Buffer *pSrcBuffer, Image *pDstImage, uint32_t regionCount, const VezBufferImageCopy *pRegions);
    // void CmdCopyImageToBuffer(Image *pSrcImage, Buffer *pDstBuffer, uint32_t regionCount, const VezBufferImageCopy *pRegions);
    // void CmdUpdateBuffer(Buffer *pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData);
    // void CmdFillBuffer(Buffer *pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
    // void CmdClearColorImage(Image *pImage, const VkClearColorValue *pColor, uint32_t rangeCount, const VezImageSubresourceRange *pRanges);
    // void CmdClearDepthStencilImage(Image *pImage, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange *pRanges);
    // void CmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects);
    // void CmdResolveImage(Image *pSrcImage, Image *pDstImage, uint32_t regionCount, const VezImageResolve *pRegions);
    // void CmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask);
    // void CmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask);

private:
    VulkanCommandPool *m_pool;
    bool               m_isRecording      = false;
    bool               m_submittedToQueue = false;
};
} // namespace vkl

#endif // COMMANDBUFFER_H_
