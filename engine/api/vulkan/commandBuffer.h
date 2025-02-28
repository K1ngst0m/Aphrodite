#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/vulkan/descriptorSet.h"
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
    Image* image{};
    std::optional<::vk::ImageLayout> layout;
    std::optional<::vk::AttachmentLoadOp> loadOp;
    std::optional<::vk::AttachmentStoreOp> storeOp;
    std::optional<::vk::ClearValue> clear;
};

struct RenderingInfo
{
    std::vector<AttachmentInfo> colors = {};
    AttachmentInfo depth = {};
    std::optional<::vk::Rect2D> renderArea = {};
};

struct ImageBlitInfo
{
    ::vk::Offset3D offset = {};
    ::vk::Offset3D extent = {};

    uint32_t level = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
};

struct ImageCopyInfo
{
    ::vk::Offset3D offset = {};
    ::vk::ImageSubresourceLayers subResources = { {}, 0, 0, 1 };
};

struct BufferBarrier
{
    Buffer* pBuffer;
    ResourceState currentState;
    ResourceState newState;
    QueueType queueType;
    uint8_t acquire;
    uint8_t release;
};

struct ImageBarrier
{
    Image* pImage;
    ResourceState currentState;
    ResourceState newState;
    QueueType queueType;
    uint8_t acquire;
    uint8_t release;
    uint8_t subresourceBarrier;
    uint8_t mipLevel;
    uint16_t arrayLayer;
};

class CommandBuffer : public ResourceHandle<::vk::CommandBuffer>
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
        struct Graphics
        {
            std::optional<VertexInput> vertexInput;
            PrimitiveTopology topology = PrimitiveTopology::TriangleList;

            struct IndexState
            {
                ::vk::Buffer buffer;
                std::size_t offset;
                IndexType indexType;
                bool dirty = false;
            } index;

            struct VertexState
            {
                ::vk::Buffer buffers[VULKAN_NUM_VERTEX_BUFFERS] = {};
                std::size_t offsets[VULKAN_NUM_VERTEX_BUFFERS] = {};
                std::bitset<32> dirty = 0;
            } vertex;

            CullMode cullMode = CullMode::None;
            WindingMode frontFace = WindingMode::CCW;
            PolygonMode polygonMode = PolygonMode::Fill;

            std::vector<AttachmentInfo> color = {};
            AttachmentInfo depth = {};

            struct DepthState
            {
                bool enable = false;
                bool write = false;
                bool stencil = false;
                CompareOp compareOp = CompareOp::Always;
            };
            DepthState depthState = {};
            uint32_t sampleCount = 1;
        } graphics;

        struct ResourceBindings
        {
            std::bitset<8> setBit = 0;
            std::bitset<32> dirtyBinding[VULKAN_NUM_DESCRIPTOR_SETS] = {};
            std::bitset<32> setBindingBit[VULKAN_NUM_DESCRIPTOR_SETS] = {};
            DescriptorUpdateInfo bindings[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS] = {};
            uint8_t pushConstantData[VULKAN_PUSH_CONSTANT_SIZE] = {};
            DescriptorSet* sets[VULKAN_NUM_DESCRIPTOR_SETS] = {};
            // TODO
            bool dirtyPushConstant = false;
        } resourceBindings = {};

        ShaderProgram* pProgram = {};
    };

public:
    CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue);
    ~CommandBuffer();

public:
    Result begin(::vk::CommandBufferUsageFlags flags = {});
    Result end();
    Result reset();

public:
    void beginRendering(const std::vector<Image*>& colors, Image* depth = nullptr);
    void beginRendering(const RenderingInfo& renderingInfo);
    void endRendering();

    void setResource(const std::vector<Sampler*>& samplers, uint32_t set, uint32_t binding);
    void setResource(const std::vector<Image*>& images, uint32_t set, uint32_t binding);
    void setResource(const std::vector<Buffer*>& buffers, uint32_t set, uint32_t binding);

    void pushConstant(const void* pData, uint32_t offset, uint32_t size);
    void setProgram(ShaderProgram* pProgram)
    {
        m_commandState.pProgram = pProgram;
    }
    void setVertexInput(const VertexInput& inputInfo)
    {
        m_commandState.graphics.vertexInput = inputInfo;
    }
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, std::size_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, std::size_t offset = 0, IndexType indexType = IndexType::UINT32);

    void setDepthState(const CommandState::Graphics::DepthState& state);

public:
    void drawIndexed(DrawIndexArguments args);
    void dispatch(Buffer* pBuffer, std::size_t offset = 0);
    void dispatch(DispatchArguments args);
    void draw(DrawArguments args);
    void draw(DispatchArguments args);
    void draw(Buffer* pBuffer, std::size_t offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(::vk::DrawIndirectCommand));

public:
    void beginDebugLabel(const DebugLabel& label);
    void insertDebugLabel(const DebugLabel& label);
    void endDebugLabel();

    void resetQueryPool(::vk::QueryPool pool, uint32_t first = 0, uint32_t count = 1);
    void writeTimeStamp(::vk::PipelineStageFlagBits stage, ::vk::QueryPool pool, uint32_t queryIndex);

public:
    void insertBarrier(const std::vector<ImageBarrier>& pImageBarriers)
    {
        insertBarrier({}, pImageBarriers);
    }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers)
    {
        insertBarrier(pBufferBarriers, {});
    }
    void insertBarrier(const std::vector<BufferBarrier>& pBufferBarriers,
                       const std::vector<ImageBarrier>& pImageBarriers);
    void transitionImageLayout(Image* pImage, ResourceState newState);

public:
    void updateBuffer(Buffer* pBuffer, MemoryRange range, const void* data);
    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, MemoryRange range);
    void copyImage(Image* srcImage, Image* dstImage, Extent3D extent = {}, const ImageCopyInfo& srcCopyInfo = {},
                   const ImageCopyInfo& dstCopyInfo = {});
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<::vk::BufferImageCopy>& regions = {});
    void blitImage(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo = {},
                   const ImageBlitInfo& dstBlitInfo = {}, ::vk::Filter filter = ::vk::Filter::eLinear);

private:
    void flushComputeCommand();
    void flushGraphicsCommand();
    void flushDescriptorSet();
    void initDynamicGraphicsState();
    CommandState m_commandState = {};

private:
    Device* m_pDevice = {};
    Queue* m_pQueue = {};
    RecordState m_state = {};
};
} // namespace aph::vk

#endif // COMMANDBUFFER_H_
