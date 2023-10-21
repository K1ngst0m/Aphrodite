#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{

class Device;
class Pipeline;
class Buffer;
class Image;
class ImageView;
class Sampler;
class Queue;
class DescriptorSet;

struct CommandPoolCreateInfo
{
    Queue* queue     = {};
    bool   transient = {false};
};

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

struct BufferBarrier
{
    Buffer*       pBuffer;
    ResourceState currentState;
    ResourceState newState;
    QueueType     queueType;
    uint8_t       acquire;
    uint8_t       release;
};

struct ImageBarrier
{
    Image*        pImage;
    ResourceState currentState;
    ResourceState newState;
    QueueType     queueType;
    uint8_t       acquire;
    uint8_t       release;
    uint8_t       subresourceBarrier;
    uint8_t       mipLevel;
    uint16_t      arrayLayer;
};

struct CommandState
{
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
        uint8_t                        pushConstantData[VULKAN_PUSH_CONSTANT_SIZE];
        uint32_t                       dirty = 0;
    };

    struct IndexState
    {
        VkBuffer     buffer;
        VkDeviceSize offset;
        VkIndexType  indexType;
    };

    struct VertexBindingState
    {
        VkBuffer     buffers[VULKAN_NUM_VERTEX_BUFFERS];
        VkDeviceSize offsets[VULKAN_NUM_VERTEX_BUFFERS];
        uint32_t     dirty = 0;
    };

    Pipeline*                     pPipeline        = {};
    VkViewport                    viewport         = {};
    VkRect2D                      scissor          = {};
    std::vector<AttachmentInfo>   colorAttachments = {};
    std::optional<AttachmentInfo> depthAttachment  = {};
    ResourceBindings              resourceBindings = {};
    IndexState                    index            = {};
    VertexBindingState            vertexBinding    = {};
};

class CommandBuffer : public ResourceHandle<VkCommandBuffer>
{
public:
    CommandBuffer(Device* pDevice, VkCommandPool pool, VkCommandBuffer handle, Queue* pQueue);
    ~CommandBuffer();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

    void bindBuffer(uint32_t set, uint32_t binding, ResourceType type, Buffer* buffer, VkDeviceSize offset = 0,
                    VkDeviceSize size = VK_WHOLE_SIZE);
    void bindTexture(uint32_t set, uint32_t binding, ResourceType type, ImageView* imageview, VkImageLayout layout,
                     Sampler* sampler = nullptr);

    void beginRendering(VkRect2D renderArea, const std::vector<Image*>& colors, Image* depth = nullptr);
    void beginRendering(VkRect2D renderArea, const std::vector<AttachmentInfo>& colors, const AttachmentInfo& depth);
    void endRendering();

    void setViewport(const VkExtent2D& extent);
    void setScissor(const VkExtent2D& extent);
    void setViewport(const VkViewport& viewport);
    void setScissor(const VkRect2D& scissor);
    void bindDescriptorSet(const std::vector<DescriptorSet*>& descriptorSets, uint32_t firstSet = 0);
    void bindDescriptorSet(uint32_t firstSet, uint32_t descriptorSetCount, const DescriptorSet* pDescriptorSets,
                           uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffset = nullptr);
    void bindPipeline(Pipeline* pPipeline);
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, uint32_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void pushConstants(uint32_t offset, uint32_t size, const void* pValues);

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                     uint32_t firstInstance);
    void dispatch(Buffer* pBuffer, VkDeviceSize offset = 0);
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void draw(Buffer* pBuffer, VkDeviceSize offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(VkDrawIndirectCommand));

    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, MemoryRange range);
    void insertBarrier(const std::vector<ImageBarrier>& pImageBarriers) { insertBarrier({}, pImageBarriers); }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers) { insertBarrier(pBufferBarriers, {}); }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers,
                       const std::vector<ImageBarrier>&  pImageBarriers);
    void transitionImageLayout(Image* pImage, ResourceState newState);
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions = {});
    void copyImage(Image* srcImage, Image* dstImage);
    void blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter = VK_FILTER_LINEAR);

    void beginDebugLabel(const DebugLabel& label);
    void endDebugLabel();
    void insertDebugLabel(const DebugLabel& label);

private:
    void                   flushComputeCommand();
    void                   flushGraphicsCommand();
    Device*                m_pDevice          = {};
    Queue*                 m_pQueue           = {};
    const VolkDeviceTable* m_pDeviceTable     = {};
    VkCommandPool          m_pool             = {};
    CommandBufferState     m_state            = {};
    bool                   m_submittedToQueue = {false};

private:
    CommandState m_commandState = {};
};
}  // namespace aph::vk

#endif  // COMMANDBUFFER_H_
