#pragma once

#include "api/vulkan/device.h"
#include "passResource.h"
#include "resource/resourceLoader.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph;
using ExecuteCallBack           = std::function<void(vk::CommandBuffer*)>;
using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
using ClearColorCallBack        = std::function<bool(uint32_t, VkClearColorValue*)>;
using ResourceLoadCallback      = std::function<void()>;

// Concepts for resource types
template <typename T>
concept ImageResourceType = std::is_same_v<T, vk::Image*> || std::is_same_v<T, ImageLoadInfo>;

template <typename T>
concept BufferResourceType = std::is_same_v<T, vk::Buffer*> || std::is_same_v<T, BufferLoadInfo>;

template <typename T>
concept ResourceUsageType = std::is_same_v<T, ImageUsage> || std::is_same_v<T, BufferUsage>;

class RenderPass
{
public:
    class Builder
    {
        RenderPass* m_pass;

    public:
        Builder(const Builder&)                    = delete;
        Builder(Builder&&)                         = default;
        auto operator=(const Builder&) -> Builder& = delete;
        auto operator=(Builder&&) -> Builder&      = default;

        explicit Builder(RenderPass* pass)
            : m_pass(pass)
        {
        }

        template <typename ResourceType, ResourceUsageType UsageType>
        auto resource(const std::string& name, const ResourceType& resourceInfo, UsageType usage, bool shared = false)
            -> Builder&;

        template <ResourceUsageType UsageType>
        auto output(const std::string& name, UsageType usage) -> Builder&;

        auto attachment(const std::string& name, const RenderPassAttachmentInfo& info, bool isDepth = false)
            -> Builder&;

        auto shader(const std::string& name, const ShaderLoadInfo& loadInfo, ResourceLoadCallback&& callback = nullptr)
            -> Builder&;
        auto build() -> RenderPass*;

        auto execute(ExecuteCallBack&& callback) -> Builder&;
        auto resetExecute() -> Builder&;
        auto execute(const std::string& shaderName, ExecuteCallBack&& callback) -> Builder&;
        auto markResourceAsShared(const std::string& resourceName) -> Builder&;
    };

    auto configure() -> Builder;

public:
    RenderPass(RenderGraph* pGraph, QueueType queueType, std::string_view name);

    auto addBufferIn(const std::string& name, vk::Buffer* pBuffer, BufferUsage usage) -> PassBufferResource*;
    auto addBufferIn(const std::string& name, const BufferLoadInfo& loadInfo, BufferUsage usage) -> PassBufferResource*;
    auto addBufferOut(const std::string& name, BufferUsage usage = BufferUsage::Storage) -> PassBufferResource*;
    auto addTextureIn(const std::string& name, vk::Image* pImage = nullptr, ImageUsage usage = ImageUsage::Sampled)
        -> PassImageResource*;
    auto addTextureIn(const std::string& name, const ImageLoadInfo& loadInfo, ImageUsage usage = ImageUsage::Sampled)
        -> PassImageResource*;
    auto addTextureOut(const std::string& name, ImageUsage usage = ImageUsage::Storage) -> PassImageResource*;

    auto setColorOut(const std::string& name, const RenderPassAttachmentInfo& info) -> PassImageResource*;
    auto setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info) -> PassImageResource*;

    void addShader(const std::string& name, const ShaderLoadInfo& loadInfo, ResourceLoadCallback callback = nullptr);

    auto getQueueType() const -> QueueType;

    void resetCommand();
    void recordCommand(const std::string& shaderName, ExecuteCallBack&& callback);
    void recordExecute(ExecuteCallBack&& cb);
    void recordClear(ClearColorCallBack&& cb);
    void recordDepthStencil(ClearDepthStencilCallBack&& cb);
    void setExecutionCondition(std::function<bool()>&& condition);
    void setCulled(bool culled);
    auto shouldExecute() const -> bool;

private:
    void markResourceAsShared(const std::string& resourceName);

private:
    friend class RenderGraph;
    friend class FrameComposer;
    struct
    {
        HashMap<PassResource*, ResourceState> resourceStateMap;
        SmallVector<PassBufferResource*> storageBufferIn;
        SmallVector<PassBufferResource*> storageBufferOut;
        SmallVector<PassBufferResource*> uniformBufferIn;
        SmallVector<PassImageResource*> textureIn;
        SmallVector<PassImageResource*> textureOut;
        SmallVector<PassImageResource*> colorOut;
        PassImageResource* depthOut = {};
    } m_resource;

private:
    struct RecordInfo
    {
        std::string shaderName;
        ExecuteCallBack callback;
    };
    SmallVector<RecordInfo> m_recordList;

    ExecuteCallBack m_executeCB;

    ClearDepthStencilCallBack m_clearDepthStencilCB;
    ClearColorCallBack m_clearColorCB;

    RenderGraph* m_pRenderGraph = {};
    QueueType m_queueType       = {};
    std::string m_name;

    enum class ExecutionMode : uint8_t
    {
        eAlways, // Pass always executes
        eConditional, // Pass executes based on condition
        eCulled // Pass is excluded from execution
    };

    ExecutionMode m_executionMode = ExecutionMode::eAlways;
    std::function<bool()> m_conditionCallback;
};

template <ResourceUsageType UsageType>
inline auto RenderPass::Builder::output(const std::string& name, UsageType usage) -> Builder&
{
    if constexpr (std::is_same_v<UsageType, ImageUsage>)
    {
        m_pass->addTextureOut(name, usage);
    }
    else if constexpr (std::is_same_v<UsageType, BufferUsage>)
    {
        m_pass->addBufferOut(name, usage);
    }
    return *this;
}

template <typename ResourceType, ResourceUsageType UsageType>
inline auto RenderPass::Builder::resource(const std::string& name, const ResourceType& resourceInfo, UsageType usage,
                                          bool shared) -> Builder&
{
    if constexpr (std::is_same_v<UsageType, ImageUsage> &&
                  (ImageResourceType<ResourceType> || std::is_same_v<ResourceType, std::nullptr_t>))
    {
        m_pass->addTextureIn(name, resourceInfo, usage);
    }
    else if constexpr (std::is_same_v<UsageType, BufferUsage> &&
                       (BufferResourceType<ResourceType> || std::is_same_v<ResourceType, std::nullptr_t>))
    {
        m_pass->addBufferIn(name, resourceInfo, usage);
    }
    else
    {
        static_assert(dependent_false_v<ResourceType>, "Invalid resource type");
    }

    if (shared)
    {
        m_pass->markResourceAsShared(name);
    }
    return *this;
}

inline auto RenderPass::Builder::resetExecute() -> Builder&
{
    m_pass->resetCommand();
    return *this;
}

inline auto RenderPass::Builder::execute(const std::string& shaderName, ExecuteCallBack&& callback) -> Builder&
{
    m_pass->recordCommand(shaderName, std::move(callback));
    return *this;
}

} // namespace aph
