#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "device.h"

namespace vkl
{

struct RenderPassBeginInfo
{
    VulkanRenderPass *pRenderPass;
    VulkanFramebuffer *pFramebuffer;
    VkRect2D renderArea;
    uint32_t clearValueCount;
    const VkClearValue *pClearValues;
};

enum class CommandBufferState
{
    INITIAL,
    RECORDING,
    EXECUTABLE,
    PENDING,
    INVALID,
};

class VulkanCommandBuffer : public ResourceHandle<VkCommandBuffer>
{
public:
    VulkanCommandBuffer(VulkanCommandPool *pool, VkCommandBuffer handle, uint32_t queueFamilyIndices);

    ~VulkanCommandBuffer();

    VulkanCommandPool *getPool();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

    void cmdBeginRenderPass(const RenderPassBeginInfo *pBeginInfo);
    void cmdNextSubpass();
    void cmdEndRenderPass();
    void cmdSetViewport(VkViewport *viewport);
    void cmdSetSissor(VkRect2D *scissor);
    void cmdBindDescriptorSet(VulkanPipeline *pPipeline, uint32_t firstSet, uint32_t descriptorSetCount,
                              const VkDescriptorSet *pDescriptorSets);
    void cmdBindPipeline(VulkanPipeline *pPipeline);
    void cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VulkanBuffer *pBuffer,
                              const VkDeviceSize *pOffsets);
    void cmdBindIndexBuffers(const VulkanBuffer *pBuffer, VkDeviceSize offset, VkIndexType indexType);
    void cmdPushConstants(VkPipelineLayout layout, VkShaderStageFlags stage, uint32_t offset,
                          uint32_t size, const void *pValues);
    void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                        uint32_t vertexOffset, uint32_t firstInstance);
    void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                 uint32_t firstInstance);
    void cmdCopyBuffer(VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size);
    void cmdTransitionImageLayout(
        VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    void cmdCopyBufferToImage(VulkanBuffer *buffer, VulkanImage *image);
    void cmdCopyImage(VulkanImage *srcImage, VulkanImage *dstImage);
    void cmdImageMemoryBarrier(VulkanImage *image, VkAccessFlags srcAccessMask,
                               VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout,
                               VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask,
                               VkImageSubresourceRange subresourceRange);
    void cmdBlitImage(VulkanImage *srcImage, VkImageLayout srcImageLayout, VulkanImage *dstImage,
                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions,
                      VkFilter filter = VK_FILTER_LINEAR);

    uint32_t getQueueFamilyIndices();

private:
    VulkanCommandPool *m_pool;
    CommandBufferState m_state;
    bool m_submittedToQueue = false;
    uint32_t m_queueFamilyType;
};
}  // namespace vkl

#endif  // COMMANDBUFFER_H_
