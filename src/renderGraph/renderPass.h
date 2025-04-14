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

struct BufferResourceInfo
{
    std::string name;
    ResourceLoadCallback preCallback;
    ResourceLoadCallback postCallback;
    bool shared = false;
    std::variant<vk::Buffer*, BufferLoadInfo> resource;
    BufferUsage usage;
};

struct ImageResourceInfo
{
    std::string name;
    ResourceLoadCallback preCallback;
    ResourceLoadCallback postCallback;
    bool shared = false;
    std::variant<vk::Image*, ImageLoadInfo> resource;
    ImageUsage usage;
};

struct ShaderResourceInfo
{
    std::string name;
    ResourceLoadCallback preCallback;
    ResourceLoadCallback postCallback;
    bool shared = false;
    std::variant<vk::ShaderProgram*, ShaderLoadInfo> resource;
};

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

        auto input(const BufferResourceInfo& info) -> Builder&;
        auto input(const ImageResourceInfo& info) -> Builder&;
        auto output(const std::string& name, BufferUsage usage) -> Builder&;
        auto output(const std::string& name, ImageUsage usage) -> Builder&;
        auto shader(const ShaderResourceInfo& info) -> Builder&;

        auto attachment(const std::string& name, const RenderPassAttachmentInfo& info, bool isDepth = false)
            -> Builder&;

        auto build() -> RenderPass*;

        auto execute(ExecuteCallBack&& callback) -> Builder&;
        auto resetExecute() -> Builder&;
        auto execute(const std::string& name, ExecuteCallBack&& callback) -> Builder&;
        auto markResourceAsShared(const std::string& resourceName) -> Builder&;
    };

    auto configure() -> Builder;

public:
    RenderPass(RenderGraph* pGraph, QueueType queueType, std::string_view name);

    auto addBufferIn(const BufferResourceInfo& info) -> PassBufferResource*;
    auto addBufferOut(const std::string& name, BufferUsage usage = BufferUsage::Storage) -> PassBufferResource*;
    auto addTextureIn(const ImageResourceInfo& info) -> PassImageResource*;
    auto addTextureOut(const std::string& name, ImageUsage usage = ImageUsage::Storage) -> PassImageResource*;
    auto addShader(const ShaderResourceInfo& info) -> void;

    auto setColorOut(const std::string& name, const RenderPassAttachmentInfo& info) -> PassImageResource*;
    auto setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info) -> PassImageResource*;

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

inline auto RenderPass::Builder::resetExecute() -> Builder&
{
    m_pass->resetCommand();
    return *this;
}

inline auto RenderPass::Builder::execute(const std::string& name, ExecuteCallBack&& callback) -> Builder&
{
    m_pass->recordCommand(name, std::move(callback));
    return *this;
}

inline auto RenderPass::Builder::input(const BufferResourceInfo& info) -> Builder&
{
    m_pass->addBufferIn(info);
    return *this;
}

inline auto RenderPass::Builder::input(const ImageResourceInfo& info) -> Builder&
{
    m_pass->addTextureIn(info);
    return *this;
}

inline auto RenderPass::Builder::output(const std::string& name, BufferUsage usage) -> Builder&
{
    BufferResourceInfo info;
    info.name  = name;
    info.usage = usage;
    m_pass->addBufferOut(info.name, info.usage);
    return *this;
}

inline auto RenderPass::Builder::output(const std::string& name, ImageUsage usage) -> Builder&
{
    ImageResourceInfo info;
    info.name  = name;
    info.usage = usage;
    m_pass->addTextureOut(info.name, info.usage);
    return *this;
}

inline auto RenderPass::Builder::shader(const ShaderResourceInfo& info) -> Builder&
{
    m_pass->addShader(info);
    return *this;
}

inline auto RenderPass::Builder::markResourceAsShared(const std::string& resourceName) -> Builder&
{
    m_pass->markResourceAsShared(resourceName);
    return *this;
}

} // namespace aph
