#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "device.h"

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
    void cmdSetViewport(VkViewport *viewport);
    void cmdSetSissor(VkRect2D *scissor);
    void cmdBindDescriptorSet(VkPipelineBindPoint    bindPoint,
                              VkPipelineLayout       layout,
                              uint32_t               firstSet,
                              uint32_t               descriptorSetCount,
                              const VkDescriptorSet *pDescriptorSets);
    void cmdBindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
    void cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VulkanBuffer *pBuffer, const VkDeviceSize *pOffsets);
    void cmdBindIndexBuffers(const VulkanBuffer *pBuffer, VkDeviceSize offset, VkIndexType indexType);
    void cmdPushConstants(VkPipelineLayout layout, VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void *pValues);
    void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
    void cmdCopyBuffer(VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size);
    void cmdTransitionImageLayout(VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    void cmdCopyBufferToImage(VulkanBuffer *buffer, VulkanImage *image);
    void cmdCopyImage(VulkanImage *srcImage, VulkanImage *dstImage);
private:
    VulkanCommandPool *m_pool;
    bool               m_isRecording      = false;
    bool               m_submittedToQueue = false;
};
} // namespace vkl

#endif // COMMANDBUFFER_H_
