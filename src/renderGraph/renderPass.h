#pragma once

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph;
class RenderPass;

using ExecuteCallBack = std::function<void(vk::CommandBuffer*)>;
using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
using ClearColorCallBack = std::function<bool(uint32_t, VkClearColorValue*)>;
using ResourceLoadCallback = std::function<void()>;

enum class PassResourceFlagBits
{
    None = 0,
    External = (1 << 0),
    Shared = (1 << 1), // Resource is shared across frames
};
using PassResourceFlags = Flags<PassResourceFlagBits>;

class PassResource
{
public:
    virtual ~PassResource() = default;

    enum class Type
    {
        Image,
        Buffer,
    };

    PassResource(Type type)
        : m_type(type)
    {
    }

    void addWritePass(RenderPass* pPass)
    {
        m_writePasses.insert(pPass);
    }
    void addReadPass(RenderPass* pPass)
    {
        m_readPasses.insert(pPass);
    }
    void addAccessFlags(::vk::AccessFlagBits2 flag)
    {
        m_accessFlags |= flag;
    }
    void addFlags(PassResourceFlags flag)
    {
        m_flags |= flag;
    }

    const HashSet<RenderPass*>& getReadPasses() const
    {
        return m_readPasses;
    }
    const HashSet<RenderPass*>& getWritePasses() const
    {
        return m_writePasses;
    }

    Type getType() const
    {
        return m_type;
    }
    PassResourceFlags getFlags() const
    {
        return m_flags;
    }
    ::vk::AccessFlags2 getAccessFlags() const
    {
        return m_accessFlags;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    void setName(std::string name)
    {
        m_name = std::move(name);
    }

protected:
    Type m_type;
    HashSet<RenderPass*> m_writePasses;
    HashSet<RenderPass*> m_readPasses;
    ::vk::AccessFlags2 m_accessFlags = {};
    PassResourceFlags m_flags = PassResourceFlagBits::None;
    std::string m_name;
};

struct RenderPassAttachmentInfo
{
    vk::ImageCreateInfo createInfo = {};
    vk::AttachmentInfo attachmentInfo = {};
};

class PassImageResource : public PassResource
{
public:
    PassImageResource(Type type)
        : PassResource(type)
    {
    }
    void setInfo(const RenderPassAttachmentInfo& info)
    {
        m_info = info;
    }
    void addUsage(ImageUsageFlags usage)
    {
        m_usage |= usage;
    }

    const RenderPassAttachmentInfo& getInfo() const
    {
        return m_info;
    }
    ImageUsageFlags getUsage() const
    {
        return m_usage;
    }

private:
    RenderPassAttachmentInfo m_info;
    ImageUsageFlags m_usage = {};
};

class PassBufferResource : public PassResource
{
public:
    PassBufferResource(Type type)
        : PassResource(type)
    {
    }
    void addInfo(const vk::BufferCreateInfo& info)
    {
        m_info = info;
    }
    void addUsage(BufferUsageFlags usage)
    {
        m_usage |= usage;
    }

    const vk::BufferCreateInfo& getInfo() const
    {
        return m_info;
    }
    BufferUsageFlags getUsage() const
    {
        return m_usage;
    }

private:
    vk::BufferCreateInfo m_info;
    BufferUsageFlags m_usage;
};

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

    void addShader(const std::string& name, const ShaderLoadInfo& loadInfo,
                         ResourceLoadCallback callback = nullptr);

    QueueType getQueueType() const;

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
    ExecuteCallBack m_executeCB;
    ClearDepthStencilCallBack m_clearDepthStencilCB;
    ClearColorCallBack m_clearColorCB;

    RenderGraph* m_pRenderGraph = {};
    QueueType m_queueType = {};
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
