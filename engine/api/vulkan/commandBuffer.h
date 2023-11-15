#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/vulkan/shader.h"
#include "vkUtils.h"

namespace aph::vk
{

class CommandPool;
class Device;
class Pipeline;
class Buffer;
class Image;
class ImageView;
class Sampler;
class Queue;
class DescriptorSet;

struct AttachmentInfo
{
    Image*                             image{};
    std::optional<VkImageLayout>       layout;
    std::optional<VkAttachmentLoadOp>  loadOp;
    std::optional<VkAttachmentStoreOp> storeOp;
    std::optional<VkClearValue>        clear;
};

struct RenderingInfo
{
    std::vector<AttachmentInfo> colors     = {};
    AttachmentInfo              depth      = {};
    std::optional<VkRect2D>     renderArea = {};
};

struct ImageBlitInfo
{
    VkOffset3D offset = {};
    VkOffset3D extent = {};

    uint32_t level      = 0;
    uint32_t baseLayer  = 0;
    uint32_t layerCount = 1;
};

struct ImageCopyInfo
{
    VkOffset3D               offset       = {0, 0, 0};
    VkImageSubresourceLayers subResources = {
        .aspectMask     = VK_IMAGE_ASPECT_NONE,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };
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

class CommandBuffer : public ResourceHandle<VkCommandBuffer>
{
    enum class RecordState
    {
        Initial,
        Recording,
        Executable,
        Pending,
        Invalid,
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
            std::size_t      dynamicOffset;
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
            VkBuffer    buffer;
            std::size_t offset;
            VkIndexType indexType;
        };

        struct VertexBindingState
        {
            VertexInput inputInfo                          = {};
            VkBuffer    buffers[VULKAN_NUM_VERTEX_BUFFERS] = {};
            std::size_t offsets[VULKAN_NUM_VERTEX_BUFFERS] = {};
            uint32_t    dirty                              = 0;
        };

        Pipeline*                   pPipeline        = {};
        ShaderProgram*              pProgram         = {};
        std::vector<AttachmentInfo> colorAttachments = {};
        AttachmentInfo              depthAttachment  = {};
        ResourceBindings            resourceBindings = {};
        IndexState                  index            = {};
        VertexBindingState          vertexBinding    = {};
    };

public:
    CommandBuffer(Device* pDevice, CommandPool* pool, HandleType handle, Queue* pQueue);
    ~CommandBuffer();

public:
    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

public:
    void beginRendering(const std::vector<Image*>& colors, Image* depth = nullptr);
    void beginRendering(const RenderingInfo& renderingInfo);
    void endRendering();

    void setProgram(ShaderProgram* pProgram) { m_commandState.pProgram = pProgram; }
    void setVertexInput(const VertexInput& inputInfo) { m_commandState.vertexBinding.inputInfo = inputInfo; }
    void bindDescriptorSet(const std::vector<DescriptorSet*>& descriptorSets, uint32_t firstSet = 0);
    void bindPipeline(Pipeline* pPipeline);
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, std::size_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, std::size_t offset = 0, IndexType indexType = IndexType::UINT32);
    void pushConstants(uint32_t offset, uint32_t size, const void* pValues);

public:
    void drawIndexed(DrawIndexArguments args);
    void dispatch(Buffer* pBuffer, std::size_t offset = 0);
    void dispatch(DispatchArguments args);
    void draw(DrawArguments args);
    void draw(Buffer* pBuffer, std::size_t offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(VkDrawIndirectCommand));

public:
    void setDebugName(std::string_view debugName);
    void beginDebugLabel(const DebugLabel& label);
    void insertDebugLabel(const DebugLabel& label);
    void endDebugLabel();

    void resetQueryPool(VkQueryPool pool, uint32_t first = 0, uint32_t count = 1);
    void writeTimeStamp(VkPipelineStageFlagBits stage, VkQueryPool pool, uint32_t queryIndex);

public:
    void insertBarrier(const std::vector<ImageBarrier>& pImageBarriers) { insertBarrier({}, pImageBarriers); }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers) { insertBarrier(pBufferBarriers, {}); }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers,
                       const std::vector<ImageBarrier>&  pImageBarriers);
    void transitionImageLayout(Image* pImage, ResourceState newState);

public:
    void updateBuffer(Buffer* pBuffer, MemoryRange range, const void* data);
    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, MemoryRange range);
    void copyImage(Image* srcImage, Image* dstImage, VkExtent3D extent = {}, const ImageCopyInfo& srcCopyInfo = {},
                   const ImageCopyInfo& dstCopyInfo = {});
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions = {});
    void blitImage(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo = {},
                   const ImageBlitInfo& dstBlitInfo = {}, VkFilter filter = VK_FILTER_LINEAR);

private:
    void         flushComputeCommand();
    void         flushGraphicsCommand();
    CommandState m_commandState = {};

private:
    Device*                m_pDevice          = {};
    Queue*                 m_pQueue           = {};
    const VolkDeviceTable* m_pDeviceTable     = {};
    CommandPool*           m_pool             = {};
    RecordState            m_state            = {};
    bool                   m_submittedToQueue = {false};
};
}  // namespace aph::vk

#endif  // COMMANDBUFFER_H_
