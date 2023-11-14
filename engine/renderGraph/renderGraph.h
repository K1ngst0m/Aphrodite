#ifndef APH_RDG_H_
#define APH_RDG_H_

#include "api/vulkan/device.h"
#include "common/timer.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph;
class RenderPass;

struct PassImageInfo
{
    Extent3D extent;
    Format   format  = Format::Undefined;
    uint32_t samples = 1;
    uint32_t levels  = 1;
    uint32_t layers  = 1;
};

struct PassBufferInfo
{
    VkDeviceSize       size  = 0;
    VkBufferUsageFlags usage = 0;
};

enum PassResourceFlags
{
    PASS_RESOURCE_NONE     = 0,
    PASS_RESOURCE_EXTERNAL = (1 << 0),
};
MAKE_ENUM_FLAG(uint32_t, PassResourceFlags);

class PassResource
{
public:
    enum class Type
    {
        Image,
        Buffer,
    };

    PassResource(Type type) : m_type(type) {}

    void addWritePass(RenderPass* pPass) { m_writePasses.insert(pPass); }
    void addReadPass(RenderPass* pPass) { m_readPasses.insert(pPass); }
    void setResourceState(ResourceState state) { m_resourceState = state; }
    void addPipelineStage(VkPipelineStageFlagBits2 stage) { m_pipelineStages |= stage; }
    void addAccessFlags(VkAccessFlagBits2 flag) { m_accessFlags |= flag; }
    void addFlags(PassResourceFlags flag) { m_flags |= flag; }

    Type                  getType() const { return m_type; }
    PassResourceFlags     getFlags() const { return m_flags; }
    ResourceState         getResourceState() const { return m_resourceState; }
    VkPipelineStageFlags2 getPipelineStage() const { return m_pipelineStages; }
    VkAccessFlags2        getAccessFlags() const { return m_accessFlags; }

protected:
    Type                  m_type;
    HashSet<RenderPass*>  m_writePasses;
    HashSet<RenderPass*>  m_readPasses;
    ResourceState         m_resourceState  = ResourceState::Undefined;
    VkPipelineStageFlags2 m_pipelineStages = 0;
    VkAccessFlags2        m_accessFlags    = 0;
    PassResourceFlags     m_flags          = PASS_RESOURCE_NONE;
};

class PassImageResource : public PassResource
{
public:
    PassImageResource(Type type) : PassResource(type) {}
    void setInfo(const PassImageInfo& info) { m_info = info; }
    void addUsage(VkImageUsageFlags usage) { m_usage |= usage; }

    const PassImageInfo& getInfo() const { return m_info; }
    VkImageUsageFlags    getUsage() const { return m_usage; }

private:
    PassImageInfo     m_info  = {};
    VkImageUsageFlags m_usage = {};
};

class PassBufferResource : public PassResource
{
public:
    PassBufferResource(Type type) : PassResource(type) {}
    void addInfo(const PassBufferInfo& info) { m_info = info; }
    void addUsage(VkBufferUsageFlags usage) { m_usage |= usage; }

    const PassBufferInfo& getInfo() const { return m_info; }
    VkBufferUsageFlags    getUsage() const { return m_usage; }

private:
    PassBufferInfo     m_info;
    VkBufferUsageFlags m_usage;
};

class RenderPass
{
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name);

    PassImageResource* addTextureInput(const std::string& name, vk::Image* pImage = nullptr);
    PassImageResource* setColorOutput(const std::string& name, const PassImageInfo& info, uint32_t outIndex = 0);
    PassImageResource* setDepthStencilOutput(const std::string& name, const PassImageInfo& info);

    using ExecuteCallBack           = std::function<void(vk::CommandBuffer*)>;
    using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
    using ClearColorCallBack        = std::function<bool(uint32_t, VkClearColorValue*)>;

    void recordExecute(ExecuteCallBack&& cb) { m_executeCB = cb; }
    void recordClear(ClearColorCallBack&& cb) { m_clearColorCB = cb; }
    void recordDepthStencil(ClearDepthStencilCallBack&& cb) { m_clearDepthStencilCB = cb; }

private:
    ExecuteCallBack           m_executeCB;
    ClearDepthStencilCallBack m_clearDepthStencilCB;
    ClearColorCallBack        m_clearColorCB;

private:
    struct
    {
        SmallVector<PassImageResource*> textureIn;
        SmallVector<PassImageResource*> colorOutMap;
        PassImageResource*              depthOut  = {};
        vk::CommandPool*                pCmdPools = {};
    } m_res;

private:
    RenderGraph* m_pRenderGraph = {};
    uint32_t     m_index        = {};
    QueueType    m_queueType    = {};
    std::string  m_name;
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
    bool          hasResource(const std::string& name) const { return m_declareData.resourceMap.contains(name); }
    vk::Image*    getBuildResource(PassImageResource* pResource) const;
    vk::Buffer*   getBuildResource(PassBufferResource* pResource) const;

    void build(const std::string& output);
    void execute(const std::string& output, vk::Fence* pFence = nullptr, vk::SwapChain* pSwapChain = nullptr);

private:
    void addDependencies() {}

private:
    vk::Device* m_pDevice     = {};
    TaskManager m_taskManager = {5, "Render Graph"};

    struct
    {
        SmallVector<RenderPass*>          passes;
        HashMap<std::string, std::size_t> passMap;

        SmallVector<PassResource*>        resources;
        HashMap<std::string, std::size_t> resourceMap;
    } m_declareData;

    struct
    {
        HashMap<PassResource*, vk::Image*>  image;
        HashMap<PassResource*, vk::Buffer*> buffer;
    } m_buildData;

    struct
    {
        ThreadSafeObjectPool<PassBufferResource> passBufferResource;
        ThreadSafeObjectPool<PassImageResource>  passImageResource;
        ThreadSafeObjectPool<RenderPass>         renderPass;
    } m_resourcePool;
};

}  // namespace aph

#endif
