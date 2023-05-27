#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{

class Device;
class CommandPool;
class Pipeline;
class Buffer;
class Image;
class ImageView;
class Sampler;

enum class CommandBufferState
{
    INITIAL,
    RECORDING,
    EXECUTABLE,
    PENDING,
    INVALID,
};

struct AttachmentInfo
{
    Image*                             image{};
    std::optional<VkImageLayout>       layout;
    std::optional<VkAttachmentLoadOp>  loadOp;
    std::optional<VkAttachmentStoreOp> storeOp;
    std::optional<VkClearValue>        clear;
};

struct ResourceBinding
{
    union
    {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo  image;
        VkBufferView           bufferView;
    };
    VkDeviceSize     dynamicOffset;
    VkDescriptorType resType;
};

struct ResourceBindings
{
    std::optional<ResourceBinding> bindings[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
    uint8_t                        push_constant_data[VULKAN_PUSH_CONSTANT_SIZE];
};

struct CommandState
{
    Pipeline*                     pPipeline{};
    VkViewport                    viewport{};
    VkRect2D                      scissor{};
    std::vector<AttachmentInfo>   colorAttachments;
    std::optional<AttachmentInfo> depthAttachment;
    ResourceBindings              resourceBindings{};
};

class CommandBuffer : public ResourceHandle<VkCommandBuffer>
{
public:
    CommandBuffer(Device* pDevice, CommandPool* pool, VkCommandBuffer handle, uint32_t queueFamilyIndices);
    ~CommandBuffer();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

    void bindBuffer(uint32_t set, uint32_t binding, ResourceType type, Buffer* buffer, VkDeviceSize offset = 0,
                    VkDeviceSize size = VK_WHOLE_SIZE);
    void bindTexture(uint32_t set, uint32_t binding, ResourceType type, ImageView* imageview, VkImageLayout layout,
                     Sampler* sampler = nullptr);

    void setRenderTarget(const std::vector<Image*>& colors, Image* depth = nullptr);
    void setRenderTarget(const std::vector<AttachmentInfo>& colors, const AttachmentInfo& depth);
    void beginRendering(VkRect2D renderArea);
    void beginRendering(const VkRenderingInfo& renderingInfo);
    void endRendering();

    void setViewport(const VkViewport& viewport);
    void setSissor(const VkRect2D& scissor);
    void bindDescriptorSet(const std::vector<VkDescriptorSet>& pDescriptorSets, uint32_t firstSet = 0);
    void bindDescriptorSet(uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets,
                           uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffset = nullptr);
    void bindPipeline(Pipeline* pPipeline);
    void bindVertexBuffers(const Buffer* pBuffer, uint32_t firstBinding = 0, uint32_t bindingCount = 1,
                           const std::vector<VkDeviceSize>& offsets = {0});
    void bindIndexBuffers(const Buffer* pBuffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void pushConstants(uint32_t offset, uint32_t size, const void* pValues);

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                     uint32_t firstInstance);
    void dispatch(Buffer* pBuffer, VkDeviceSize offset = 0);
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void draw(Buffer* pBuffer, VkDeviceSize offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(VkDrawIndirectCommand));

    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size);
    void transitionImageLayout(Image* image, VkImageLayout newLayout,
                               VkImageSubresourceRange* pSubResourceRange = nullptr,
                               VkPipelineStageFlags     srcStageMask      = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VkPipelineStageFlags     dstStageMask      = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions = {});
    void copyImage(Image* srcImage, Image* dstImage);
    void imageMemoryBarrier(Image* image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                            VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask,
                            VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange);
    void blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter = VK_FILTER_LINEAR);

    uint32_t getQueueFamilyIndices() const;

private:
    void                   flushComputeCommand();
    void                   flushGraphicsCommand();
    Device*                m_pDevice          = {};
    const VolkDeviceTable* m_pDeviceTable     = {};
    CommandPool*           m_pool             = {};
    CommandBufferState     m_state            = {};
    bool                   m_submittedToQueue = {false};
    uint32_t               m_queueFamilyType  = {};
    CommandState           m_commandState     = {};
};
}  // namespace aph::vk

#endif  // COMMANDBUFFER_H_
