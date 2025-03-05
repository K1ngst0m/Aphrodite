#pragma once

#include "api/vulkan/descriptorSet.h"
#include "api/vulkan/shader.h"
#include "common/arrayProxy.h"
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
class BindlessResource;

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
    SmallVector<AttachmentInfo> colors = {};
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
    friend class ObjectPool<CommandBuffer>;

    enum class RecordState
    {
        Initial,
        Recording,
        Executable,
        Pending,
        Invalid,
    };

    enum class DirtyFlagBits
    {
        vertexInput = 1 << 1,
        indexState = 1 << 2,
        vertexState = 1 << 3,
        dynamicState = 1 << 4,
        pushConstant = 1 << 5,
    };
    using DirtyFlags = Flags<DirtyFlagBits>;

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
            } index;

            struct VertexState
            {
                ::vk::Buffer buffers[VULKAN_NUM_VERTEX_BUFFERS] = {};
                std::size_t offsets[VULKAN_NUM_VERTEX_BUFFERS] = {};
                std::bitset<32> dirty = {};
            } vertex;

            CullMode cullMode = CullMode::None;
            WindingMode frontFace = WindingMode::CCW;
            PolygonMode polygonMode = PolygonMode::Fill;

            SmallVector<AttachmentInfo> color = {};
            AttachmentInfo depth = {};

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
        } resourceBindings = {};

        std::unique_ptr<BindlessResource> bindlessResource;

        ShaderProgram* pProgram = {};

        DirtyFlags dirty = DirtyFlagBits::dynamicState;
    };

private:
    CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue);
    ~CommandBuffer();

public:
    Result begin(::vk::CommandBufferUsageFlags flags = {});
    Result end();
    Result reset();

public:
    void beginRendering(ArrayProxy<Image*> colors, Image* depth = nullptr);
    void beginRendering(const RenderingInfo& renderingInfo);
    void endRendering();

    void setResource(DescriptorUpdateInfo updateInfo, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Sampler*> samplers, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Image*> images, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Buffer*> buffers, uint32_t set, uint32_t binding);
    void pushConstant(const void* pData, Range range);
    void setProgram(ShaderProgram* pProgram);

public:
    void setCullMode(const CullMode mode);
    void setFrontFaceWinding(const WindingMode mode);
    void setPolygonMode(const PolygonMode mode);
    void setDepthState(DepthState state);

    // gemometry pipeline only
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, std::size_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, std::size_t offset = 0, IndexType indexType = IndexType::UINT32);
    void setVertexInput(VertexInput inputInfo);

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
    void insertBarrier(ArrayProxy<ImageBarrier> pImageBarriers);
    void insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers);
    void insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers, ArrayProxy<ImageBarrier> pImageBarriers);
    void transitionImageLayout(Image* pImage, ResourceState newState);

public:
    void update(Buffer* pBuffer, Range range, const void* data);
    void copy(Buffer* srcBuffer, Buffer* dstBuffer, Range range);
    void copy(Image* srcImage, Image* dstImage, Extent3D extent = {}, const ImageCopyInfo& srcCopyInfo = {},
              const ImageCopyInfo& dstCopyInfo = {});
    void copy(Buffer* buffer, Image* image, ArrayProxy<::vk::BufferImageCopy> regions = {});
    void blit(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo = {},
              const ImageBlitInfo& dstBlitInfo = {}, ::vk::Filter filter = ::vk::Filter::eLinear);

private:
    void setDirty(DirtyFlagBits dirtyFlagBits);

    void flushComputeCommand();
    void flushGraphicsCommand();
    void flushDescriptorSet();
    void flushDynamicGraphicsState();
    CommandState m_commandState = {};

private:
    Device* m_pDevice = {};
    Queue* m_pQueue = {};
    RecordState m_state = {};
};
} // namespace aph::vk
