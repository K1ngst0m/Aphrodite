#ifndef APH_RDG_H_
#define APH_RDG_H_

#include "api/vulkan/device.h"

#include <unordered_set>

namespace aph
{

class RenderGraph;
class RenderPass
{
    friend class RenderGraph;

public:
    RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name);

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
        vk::CommandPool* pCmdPools = {};
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

    void execute(vk::Image* pImage, vk::SwapChain* pSwapChain)
    {
        auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

        m_pRenderTarget = pImage;

        for(auto* pass : m_passes)
        {
            auto& cmdPool = pass->m_res.pCmdPools;
            if(cmdPool == nullptr)
            {
                cmdPool = m_pDevice->acquireCommandPool({queue, false});
            }
            vk::CommandBuffer* pCmd = cmdPool->allocate();
            pass->m_executeCB(pCmd);

            // submission
            {
                vk::QueueSubmitInfo submitInfo{.commandBuffers = {pCmd}};

                vk::Semaphore* renderSem  = {};
                vk::Semaphore* presentSem = {};

                vk::Fence* frameFence = m_pDevice->acquireFence();
                m_pDevice->getDeviceTable()->vkResetFences(m_pDevice->getHandle(), 1, &frameFence->getHandle());

                auto& pPresentImage = pImage;
                if(pPresentImage)
                {
                    renderSem = m_pDevice->acquireSemaphore();
                    APH_CHECK_RESULT(pSwapChain->acquireNextImage(renderSem->getHandle()));

                    presentSem = m_pDevice->acquireSemaphore();
                    submitInfo.waitSemaphores.push_back(renderSem);
                    submitInfo.signalSemaphores.push_back(presentSem);
                }

                APH_CHECK_RESULT(queue->submit({submitInfo}, frameFence));

                if(pPresentImage)
                {
                    auto pSwapchainImage = pSwapChain->getImage();

                    // transisiton && copy
                    {
                        vk::CommandBuffer* cmd = pass->m_res.pCmdPools->allocate();
                        _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
                        cmd->transitionImageLayout(pPresentImage, RESOURCE_STATE_COPY_SRC);
                        cmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_COPY_DST);

                        if(pPresentImage->getWidth() == pSwapchainImage->getWidth() &&
                           pPresentImage->getHeight() == pSwapchainImage->getHeight() &&
                           pPresentImage->getDepth() == pSwapchainImage->getDepth())
                        {
                            VK_LOG_DEBUG("copy image to swapchain.");
                            cmd->copyImage(pPresentImage, pSwapchainImage);
                        }
                        else
                        {
                            VK_LOG_DEBUG("blit image to swapchain.");
                            cmd->blitImage(pPresentImage, pSwapchainImage);
                        }

                        cmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_PRESENT);
                        _VR(cmd->end());

                        vk::QueueSubmitInfo copySubmit{.commandBuffers = {cmd}};
                        auto                fence = m_pDevice->acquireFence();
                        APH_CHECK_RESULT(queue->submit({copySubmit}, fence));
                        fence->wait();
                    }

                    APH_CHECK_RESULT(pSwapChain->presentImage(queue, {presentSem}));
                }

                frameFence->wait();
            }
        }
    }

private:
    vk::Device* m_pDevice = {};

    std::vector<RenderPass*>                  m_passes;
    std::unordered_map<std::string, uint32_t> m_renderPassMap;

    vk::Image* m_pRenderTarget = {};

    struct
    {
    } m_frameData;

    struct
    {
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;
};

}  // namespace aph

#endif
