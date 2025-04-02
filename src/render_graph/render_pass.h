#pragma once

#include "api/vulkan/device.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph;
class RenderPass;

enum class PassResourceFlagBits
{
    None = 0,
    External = (1 << 0),
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
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pGraph, QueueType queueType, std::string_view name);

    PassBufferResource* addBufferIn(const std::string& name, vk::Buffer* pBuffer, BufferUsage usage);
    PassBufferResource* addBufferOut(const std::string& name, BufferUsage usage = BufferUsage::Storage);

    PassImageResource* addTextureIn(const std::string& name, vk::Image* pImage = nullptr,
                                    ImageUsage usage = ImageUsage::Sampled);
    PassImageResource* addTextureOut(const std::string& name, ImageUsage usage = ImageUsage::Storage);

    PassImageResource* setColorOut(const std::string& name, const RenderPassAttachmentInfo& info);
    PassImageResource* setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info);

    using ExecuteCallBack = std::function<void(vk::CommandBuffer*)>;
    using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
    using ClearColorCallBack = std::function<bool(uint32_t, VkClearColorValue*)>;

    void recordExecute(ExecuteCallBack&& cb);
    void recordClear(ClearColorCallBack&& cb);
    void recordDepthStencil(ClearDepthStencilCallBack&& cb);

    QueueType getQueueType() const
    {
        return m_queueType;
    }

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

        Builder& bufferInput(const std::string& name, vk::Buffer* pBuffer = nullptr,
                             BufferUsage usage = BufferUsage::Uniform)
        {
            m_pass->addBufferIn(name, pBuffer, usage);
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

        RenderPass* build()
        {
            return m_pass;
        }
    };

    Builder configure()
    {
        return Builder(this);
    }

    void setExecutionCondition(std::function<bool()>&& condition);

    void setCulled(bool culled);

    bool shouldExecute() const;

private:
    ExecuteCallBack m_executeCB;
    ClearDepthStencilCallBack m_clearDepthStencilCB;
    ClearColorCallBack m_clearColorCB;

private:
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
    } m_res;

private:
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
