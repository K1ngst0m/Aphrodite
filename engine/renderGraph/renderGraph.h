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

struct PassImageInfo
{
    Extent3D extent = {};
    Format format = Format::Undefined;
    uint32_t samples = 1;
    uint32_t levels = 1;
    uint32_t layers = 1;
};

struct PassBufferInfo
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    PassResourceFlags flags = {};
};

class PassResource
{
public:
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
    void addPipelineStage(VkPipelineStageFlagBits2 stage)
    {
        m_pipelineStages |= stage;
    }
    void addAccessFlags(VkAccessFlagBits2 flag)
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
    VkPipelineStageFlags2 getPipelineStage() const
    {
        return m_pipelineStages;
    }
    VkAccessFlags2 getAccessFlags() const
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
    VkPipelineStageFlags2 m_pipelineStages = 0;
    VkAccessFlags2 m_accessFlags = 0;
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
    void setInfo(const PassImageInfo& info)
    {
        m_info = info;
    }
    void addUsage(::vk::ImageUsageFlags usage)
    {
        m_usage |= usage;
    }

    const PassImageInfo& getInfo() const
    {
        return m_info;
    }
    ::vk::ImageUsageFlags getUsage() const
    {
        return m_usage;
    }

private:
    PassImageInfo m_info = {};
    ::vk::ImageUsageFlags m_usage = {};
};

class PassBufferResource : public PassResource
{
public:
    PassBufferResource(Type type)
        : PassResource(type)
    {
    }
    void addInfo(const PassBufferInfo& info)
    {
        m_info = info;
    }
    void addUsage(VkBufferUsageFlags usage)
    {
        m_usage |= usage;
    }

    const PassBufferInfo& getInfo() const
    {
        return m_info;
    }
    VkBufferUsageFlags getUsage() const
    {
        return m_usage;
    }

private:
    PassBufferInfo m_info;
    VkBufferUsageFlags m_usage;
};

class RenderPass
{
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name);

    PassBufferResource* addUniformBufferIn(const std::string& name, vk::Buffer* pBuffer = nullptr);
    PassBufferResource* addStorageBufferIn(const std::string& name, vk::Buffer* pBuffer = nullptr);
    PassBufferResource* addBufferOut(const std::string& name);

    PassImageResource* addTextureIn(const std::string& name, vk::Image* pImage = nullptr);
    PassImageResource* addTextureOut(const std::string& name);
    PassImageResource* setColorOut(const std::string& name, const PassImageInfo& info);
    PassImageResource* setDepthStencilOut(const std::string& name, const PassImageInfo& info);

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
    uint32_t getIndex() const
    {
        return m_index;
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
    uint32_t m_index = {};
    QueueType m_queueType = {};
    std::string m_name;
};

class RenderGraph
{
public:
    RenderGraph(vk::Device* pDevice);
    ~RenderGraph();

    RenderPass* createPass(const std::string& name, QueueType queueType);
    RenderPass* getPass(const std::string& name);

    PassResource* importResource(const std::string& name, vk::Image* pImage);
    PassResource* importResource(const std::string& name, vk::Buffer* pBuffer);
    PassResource* getResource(const std::string& name, PassResource::Type type);
    bool hasResource(const std::string& name) const
    {
        return m_declareData.resourceMap.contains(name);
    }
    vk::Image* getBuildResource(PassImageResource* pResource) const;
    vk::Buffer* getBuildResource(PassBufferResource* pResource) const;

    void setBackBuffer(const std::string& backBuffer);

    void build(vk::SwapChain* pSwapChain = nullptr);
    void execute(vk::Fence* pFence = nullptr);
    void cleanup();

private:
    vk::Device* m_pDevice = {};

    struct
    {
        std::string backBuffer = {};

        SmallVector<RenderPass*> passes;
        HashMap<std::string, std::size_t> passMap;

        SmallVector<PassBufferResource*> bufferResources;
        SmallVector<PassImageResource*> imageResources;
        SmallVector<PassResource*> resources;
        HashMap<std::string, std::size_t> resourceMap;
    } m_declareData;

    struct
    {
        SmallVector<RenderPass*> sortedPasses;

        HashMap<RenderPass*, vk::CommandPool*> cmdPools;
        HashMap<RenderPass*, vk::CommandBuffer*> cmds;
        HashMap<RenderPass*, HashSet<RenderPass*>> passDependencyGraph;
        HashMap<RenderPass*, std::vector<vk::ImageBarrier>> imageBarriers;
        HashMap<RenderPass*, std::vector<vk::BufferBarrier>> bufferBarriers;

        HashMap<PassResource*, vk::Image*> image;
        HashMap<PassResource*, vk::Buffer*> buffer;

        vk::SwapChain* pSwapchain = {};
        vk::Fence* frameFence = {};

        std::vector<vk::QueueSubmitInfo> frameSubmitInfos{};
        std::mutex submitLock;

    } m_buildData;

    struct
    {
        ThreadSafeObjectPool<PassBufferResource> passBufferResource;
        ThreadSafeObjectPool<PassImageResource> passImageResource;
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;
};

} // namespace aph
