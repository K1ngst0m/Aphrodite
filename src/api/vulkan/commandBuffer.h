#pragma once

#include "api/vulkan/descriptorSet.h"
#include "api/vulkan/forward.h"
#include "api/vulkan/shader.h"
#include "common/arrayProxy.h"
#include "common/breadcrumbTracker.h"
#include "vkUtils.h"

namespace aph::vk
{
struct AttachmentInfo
{
    Image* image{};
    std::optional<ImageLayout> layout;
    std::optional<AttachmentLoadOp> loadOp;
    std::optional<AttachmentStoreOp> storeOp;
    std::optional<ClearValue> clear;
};

struct RenderingInfo
{
    SmallVector<AttachmentInfo> colors = {};
    AttachmentInfo depth               = {};
    std::optional<Rect2D> renderArea   = {};
};

struct ImageBlitInfo
{
    Offset3D offset = {};
    Offset3D extent = {};

    uint32_t level      = 0;
    uint32_t baseLayer  = 0;
    uint32_t layerCount = 1;
};

struct ImageCopyInfo
{
    Offset3D offset                     = {};
    ImageSubresourceLayers subResources = { .aspectMask = 0, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
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
    friend class ThreadSafeObjectPool<CommandBuffer>;
    friend class CommandBufferAllocator;

    enum class RecordState : uint8_t
    {
        Initial,
        Recording,
        Executable,
        Pending,
        Invalid,
    };

    enum class DirtyFlagBits : uint8_t
    {
        vertexInput  = 1 << 1,
        indexState   = 1 << 2,
        vertexState  = 1 << 3,
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
                std::size_t offsets[VULKAN_NUM_VERTEX_BUFFERS]  = {};
                std::bitset<32> dirty                           = {};
            } vertex;

            CullMode cullMode       = CullMode::None;
            WindingMode frontFace   = WindingMode::CCW;
            PolygonMode polygonMode = PolygonMode::Fill;

            SmallVector<AttachmentInfo> color = {};
            AttachmentInfo depth              = {};

            DepthState depthState = {};
            uint32_t sampleCount  = 1;
        } graphics;

        struct ResourceBindings
        {
            std::bitset<8> setBit                                                          = 0;
            std::bitset<32> dirtyBinding[VULKAN_NUM_DESCRIPTOR_SETS]                       = {};
            std::bitset<32> setBindingBit[VULKAN_NUM_DESCRIPTOR_SETS]                      = {};
            DescriptorUpdateInfo bindings[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS] = {};
            uint8_t pushConstantData[VULKAN_PUSH_CONSTANT_SIZE]                            = {};
            DescriptorSet* sets[VULKAN_NUM_DESCRIPTOR_SETS]                                = {};
        } resourceBindings = {};

        BindlessResource* bindlessResource = {};

        ShaderProgram* pProgram = {};

        DirtyFlags dirty = DirtyFlagBits::dynamicState;
    };

private:
    CommandBuffer(Device* pDevice, HandleType handle, Queue* pQueue, bool transient = false);
    ~CommandBuffer();

    void setDirty(DirtyFlagBits dirtyFlagBits);
    void flushComputeCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset = {});
    void flushGraphicsCommand(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset = {});
    void flushDescriptorSet(const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset);
    void flushDynamicGraphicsState();

    // Private implementation of insertBarrier that takes a breadcrumb index
    void insertBarrier(ArrayProxy<BufferBarrier> bufferBarriers, ArrayProxy<ImageBarrier> imageBarriers,
                       uint32_t barrierIndex);

public:
    // Command buffer lifecycle
    auto begin() -> Result;
    auto end() -> Result;
    auto reset() -> Result;

    void beginRendering(const RenderingInfo& renderingInfo);
    void endRendering();

    // Resource binding
    void setResource(DescriptorUpdateInfo updateInfo, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Sampler*> samplers, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Image*> images, uint32_t set, uint32_t binding);
    void setResource(ArrayProxy<Buffer*> buffers, uint32_t set, uint32_t binding);
    void pushConstant(const void* pData, Range range);
    void setProgram(ShaderProgram* pProgram);

    // Graphics state
    void setCullMode(CullMode mode);
    void setFrontFaceWinding(WindingMode mode);
    void setPolygonMode(PolygonMode mode);
    void setDepthState(DepthState state);

    // Vertex/Index binding
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, std::size_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, std::size_t offset = 0, IndexType indexType = IndexType::UINT32);
    void setVertexInput(VertexInput inputInfo);

    // Drawing commands
    void draw(DrawArguments args);
    void draw(Buffer* pBuffer, std::size_t offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(DrawIndirectCommand));
    void drawIndexed(DrawIndexArguments args);
    void draw(DispatchArguments args, const ArrayProxyNoTemporaries<uint32_t>& dynamicOffset = {});

    // Compute commands
    void dispatch(DispatchArguments args);
    void dispatch(Buffer* pBuffer, std::size_t offset = 0);

    // Debug and profiling
    void beginDebugLabel(const DebugLabel& label);
    void insertDebugLabel(const DebugLabel& label);
    void endDebugLabel();
    void resetQueryPool(QueryPool* pQueryPool, uint32_t first = 0, uint32_t count = 1);
    void writeTimeStamp(PipelineStage stage, QueryPool* pQueryPool, uint32_t queryIndex);

    // Synchronization and memory operations
    void insertBarrier(ArrayProxy<ImageBarrier> pImageBarriers);
    void insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers);
    void insertBarrier(ArrayProxy<BufferBarrier> pBufferBarriers, ArrayProxy<ImageBarrier> pImageBarriers);
    void transitionImageLayout(Image* pImage, ResourceState newState);
    void transitionImageLayout(Image* pImage, ResourceState currentState, ResourceState newState);

    // Memory operations
    void update(Buffer* pBuffer, Range range, const void* data);
    void copy(Buffer* srcBuffer, Buffer* dstBuffer, Range range);
    void copy(Image* srcImage, Image* dstImage, Extent3D extent = {}, const ImageCopyInfo& srcCopyInfo = {},
              const ImageCopyInfo& dstCopyInfo = {});
    void copy(Buffer* buffer, Image* image, ArrayProxy<BufferImageCopy> regions = {});
    void blit(Image* srcImage, Image* dstImage, const ImageBlitInfo& srcBlitInfo = {},
              const ImageBlitInfo& dstBlitInfo = {}, Filter filter = Filter::Linear);

    // Breadcrumb tracking
    auto getBreadcrumbTracker() const -> const BreadcrumbTracker&
    {
        return m_breadcrumbs;
    }

    auto getBreadcrumbTracker() -> BreadcrumbTracker&
    {
        return m_breadcrumbs;
    }

    auto generateBreadcrumbReport() const -> std::string;

private:
    CommandState m_commandState = {};
    Device* m_pDevice           = {};
    Queue* m_pQueue             = {};
    RecordState m_state         = {};
    bool m_transient            = {};
    BreadcrumbTracker m_breadcrumbs;
    uint32_t m_currentScopeIndex = UINT32_MAX;
};
} // namespace aph::vk
