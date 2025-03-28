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

class PassImageResource : public PassResource
{
public:
    PassImageResource(Type type)
        : PassResource(type)
    {
    }
    void setInfo(const vk::ImageCreateInfo& info)
    {
        m_info = info;
    }
    void addUsage(ImageUsageFlags usage)
    {
        m_usage |= usage;
    }

    const vk::ImageCreateInfo& getInfo() const
    {
        return m_info;
    }
    ImageUsageFlags getUsage() const
    {
        return m_usage;
    }

private:
    vk::ImageCreateInfo m_info = {};
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

    PassBufferResource* addUniformBufferIn(const std::string& name, vk::Buffer* pBuffer = nullptr);
    PassBufferResource* addStorageBufferIn(const std::string& name, vk::Buffer* pBuffer = nullptr);
    PassBufferResource* addBufferOut(const std::string& name);

    PassImageResource* addTextureIn(const std::string& name, vk::Image* pImage = nullptr);
    PassImageResource* addTextureOut(const std::string& name);
    PassImageResource* setColorOut(const std::string& name, const vk::ImageCreateInfo& info);
    PassImageResource* setDepthStencilOut(const std::string& name, const vk::ImageCreateInfo& info);

    using ExecuteCallBack = std::function<void(vk::CommandBuffer*)>;
    using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
    using ClearColorCallBack = std::function<bool(uint32_t, VkClearColorValue*)>;

    void recordExecute(ExecuteCallBack&& cb)
    {
        m_executeCB = cb;
    }
    void recordClear(ClearColorCallBack&& cb)
    {
        m_clearColorCB = cb;
    }
    void recordDepthStencil(ClearDepthStencilCallBack&& cb)
    {
        m_clearDepthStencilCB = cb;
    }

    QueueType getQueueType() const
    {
        return m_queueType;
    }

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
};

} // namespace aph
