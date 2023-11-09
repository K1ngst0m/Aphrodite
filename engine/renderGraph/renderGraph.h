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

struct PassResource
{
    enum class Type
    {
        Image,
        Buffer,
    };

    Type type;

    HashSet<RenderPass*> writePasses;
    HashSet<RenderPass*> readPasses;
};

struct PassImageResource : public PassResource
{
    PassImageInfo     imageInfo = {};
    VkImageUsageFlags usage     = {};
};

struct PassBufferResource : public PassResource
{
    PassBufferInfo bufferInfo;
};

class RenderPass
{
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name);

    PassImageResource* addColorOutput(const std::string& name, const PassImageInfo& info);

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
        std::vector<PassImageResource*> colorOutMap;
        HashSet<PassImageResource*>     colorOutSet;
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

    PassResource* getResource(const std::string& name, PassResource::Type type);

    void build();
    void execute(const std::string& output, vk::SwapChain* pSwapChain = nullptr);
    vk::Fence* executeAsync(const std::string& output, vk::SwapChain* pSwapChain = nullptr);

private:
    vk::Device* m_pDevice     = {};
    TaskManager m_taskManager = {5, "Render Graph"};

    std::vector<RenderPass*>          m_passes;
    HashMap<std::string, std::size_t> m_renderPassMap;

    std::vector<PassResource*>        m_passResources;
    HashMap<std::string, std::size_t> m_passResourceMap;

    HashMap<PassResource*, vk::Image*> m_buildImageResources;

    struct
    {
        double frameTime;
        double fps;
    } m_frameData;

    struct
    {
        ThreadSafeObjectPool<PassResource> passResource;
        ThreadSafeObjectPool<RenderPass>   renderPass;
    } m_resourcePool;
};

}  // namespace aph

#endif
