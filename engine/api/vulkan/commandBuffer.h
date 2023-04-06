#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{

class VulkanDevice;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanCommandPool;
class VulkanPipeline;
class VulkanBuffer;
class VulkanImage;

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

    void beginRendering(const VkRenderingInfo &renderingInfo);
    void endRendering();
    void setViewport(VkViewport *viewport);
    void setSissor(VkRect2D *scissor);
    void bindDescriptorSet(VulkanPipeline *pPipeline, uint32_t firstSet, uint32_t descriptorSetCount,
                           const VkDescriptorSet *pDescriptorSets);
    void bindPipeline(VulkanPipeline *pPipeline);
    void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VulkanBuffer *pBuffer,
                           const std::vector<VkDeviceSize> &offsets);
    void bindIndexBuffers(const VulkanBuffer *pBuffer, VkDeviceSize offset, VkIndexType indexType);
    void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stage, uint32_t offset, uint32_t size,
                       const void *pValues);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                     uint32_t firstInstance);
    void pushDescriptorSet(VulkanPipeline *pipeline, const std::vector<VkWriteDescriptorSet> &writes, uint32_t setIdx);
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void copyBuffer(VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size);
    void transitionImageLayout(VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    void copyBufferToImage(VulkanBuffer *buffer, VulkanImage *image);
    void copyImage(VulkanImage *srcImage, VulkanImage *dstImage);
    void imageMemoryBarrier(VulkanImage *image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                            VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                            VkImageSubresourceRange subresourceRange);
    void blitImage(VulkanImage *srcImage, VkImageLayout srcImageLayout, VulkanImage *dstImage,
                   VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions,
                   VkFilter filter = VK_FILTER_LINEAR);

    uint32_t getQueueFamilyIndices() const;

private:
    VulkanCommandPool *m_pool;
    CommandBufferState m_state;
    bool m_submittedToQueue = false;
    uint32_t m_queueFamilyType;
};
}  // namespace aph

#endif  // COMMANDBUFFER_H_
