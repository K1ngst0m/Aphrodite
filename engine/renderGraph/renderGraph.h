#ifndef APH_RDG_H_
#define APH_RDG_H_

#include "api/vulkan/device.h"
#include "common/timer.h"
#include "threads/taskManager.h"

namespace aph
{

class RenderGraph;
class RenderPass
{
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name);

    void addColorOutput(vk::Image* pImage);

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
        std::unordered_set<vk::Image*> colorOutMap;
        std::vector<vk::Image*>        colorOut;
        vk::CommandPool*               pCmdPools = {};
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

    RenderPass* createPass(const std::string& name, QueueType queueType);
    RenderPass* getPass(const std::string& name);

    void execute(vk::Image* pImage, vk::SwapChain* pSwapChain = nullptr);

private:
    vk::Device* m_pDevice = {};
    TaskManager m_taskManager = {5, "Render Graph"};

    std::vector<RenderPass*>                  m_passes;
    std::unordered_map<std::string, uint32_t> m_renderPassMap;

    vk::Image* m_pRenderTarget = {};

    struct
    {
        double frameTime;
        double fps;
    } m_frameData;

    struct
    {
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;
};

}  // namespace aph

#endif
