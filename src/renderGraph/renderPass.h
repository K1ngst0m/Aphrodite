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

class RenderPass
{
public:
    class Builder
    {
        RenderPass* m_pass;

    public:
        Builder(RenderPass* pass)
            : m_pass(pass)
        {
        }

        Builder& textureInput(const std::string& name, vk::Image* pImage = nullptr,
                              ImageUsage usage = ImageUsage::Sampled)
        {
            m_pass->addTextureIn(name, pImage, usage);
            return *this;
        }

        Builder& textureInput(const std::string& name, const ImageLoadInfo& loadInfo,
                              ImageUsage usage = ImageUsage::Sampled)
        {
            m_pass->addTextureIn(name, loadInfo, usage);
            return *this;
        }

        Builder& bufferInput(const std::string& name, vk::Buffer* pBuffer = nullptr,
                             BufferUsage usage = BufferUsage::Uniform)
        {
            m_pass->addBufferIn(name, pBuffer, usage);
            return *this;
        }

        Builder& bufferInput(const std::string& name, const BufferLoadInfo& loadInfo,
                             BufferUsage usage = BufferUsage::Uniform)
        {
            m_pass->addBufferIn(name, loadInfo, usage);
            return *this;
        }

        Builder& textureOutput(const std::string& name, ImageUsage usage = ImageUsage::Storage)
        {
            m_pass->addTextureOut(name, usage);
            return *this;
        }

        Builder& bufferOutput(const std::string& name, BufferUsage usage = BufferUsage::Storage)
        {
            m_pass->addBufferOut(name, usage);
            return *this;
        }

        Builder& colorOutput(const std::string& name, const RenderPassAttachmentInfo& info)
        {
            m_pass->setColorOut(name, info);
            return *this;
        }

        Builder& depthOutput(const std::string& name, const RenderPassAttachmentInfo& info)
        {
            m_pass->setDepthStencilOut(name, info);
            return *this;
        }

        Builder& execute(ExecuteCallBack&& cb)
        {
            m_pass->recordExecute(std::move(cb));
            return *this;
        }

        Builder& sharedTextureInput(const std::string& name, vk::Image* pImage = nullptr,
                                    ImageUsage usage = ImageUsage::Sampled)
        {
            m_pass->addTextureIn(name, pImage, usage);
            m_pass->markResourceAsShared(name);
            return *this;
        }

        Builder& sharedTextureInput(const std::string& name, const ImageLoadInfo& loadInfo,
                                    ImageUsage usage = ImageUsage::Sampled)
        {
            m_pass->addTextureIn(name, loadInfo, usage);
            m_pass->markResourceAsShared(name);
            return *this;
        }

        Builder& sharedBufferInput(const std::string& name, vk::Buffer* pBuffer = nullptr,
                                   BufferUsage usage = BufferUsage::Uniform)
        {
            m_pass->addBufferIn(name, pBuffer, usage);
            m_pass->markResourceAsShared(name);
            return *this;
        }

        Builder& sharedBufferInput(const std::string& name, const BufferLoadInfo& loadInfo,
                                   BufferUsage usage = BufferUsage::Uniform)
        {
            m_pass->addBufferIn(name, loadInfo, usage);
            m_pass->markResourceAsShared(name);
            return *this;
        }

        // Add shader to be loaded after resources
        Builder& shader(const std::string& name, const ShaderLoadInfo& loadInfo,
                        ResourceLoadCallback callback = nullptr)
        {
            m_pass->addShader(name, loadInfo, callback);
            return *this;
        }

        Builder& markResourceAsShared(const std::string& resourceName)
        {
            m_pass->markResourceAsShared(resourceName);
            return *this;
        }

        RenderPass* build()
        {
            return m_pass;
        }
    };

    Builder configure()
    {
        return Builder(this);
    }

public:
    RenderPass(RenderGraph* pGraph, QueueType queueType, std::string_view name);

    PassBufferResource* addBufferIn(const std::string& name, vk::Buffer* pBuffer, BufferUsage usage);
    PassBufferResource* addBufferIn(const std::string& name, const BufferLoadInfo& loadInfo, BufferUsage usage);
    PassBufferResource* addBufferOut(const std::string& name, BufferUsage usage = BufferUsage::Storage);
    PassImageResource* addTextureIn(const std::string& name, vk::Image* pImage = nullptr,
                                    ImageUsage usage = ImageUsage::Sampled);
    PassImageResource* addTextureIn(const std::string& name, const ImageLoadInfo& loadInfo,
                                    ImageUsage usage = ImageUsage::Sampled);
    PassImageResource* addTextureOut(const std::string& name, ImageUsage usage = ImageUsage::Storage);

    PassImageResource* setColorOut(const std::string& name, const RenderPassAttachmentInfo& info);
    PassImageResource* setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info);

    void addShader(const std::string& name, const ShaderLoadInfo& loadInfo, ResourceLoadCallback callback = nullptr);

    QueueType getQueueType() const;

    void pushCommands(const std::string& shaderName, ExecuteCallBack&& callback)
    {
        m_recordList.push_back({.shaderName = shaderName, .callback = std::move(callback)});
    }
    void recordExecute(ExecuteCallBack&& cb);
    void recordClear(ClearColorCallBack&& cb);
    void recordDepthStencil(ClearDepthStencilCallBack&& cb);
    void setExecutionCondition(std::function<bool()>&& condition);
    void setCulled(bool culled);
    bool shouldExecute() const;

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

    enum class ExecutionMode
    {
        Always, // Pass always executes
        Conditional, // Pass executes based on condition
        Culled // Pass is excluded from execution
    };

    ExecutionMode m_executionMode = ExecutionMode::Always;
    std::function<bool()> m_conditionCallback;
};

} // namespace aph
