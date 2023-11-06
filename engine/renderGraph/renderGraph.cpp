#include "renderGraph.h"
#include "threads/taskManager.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name) :
    m_pRenderGraph(pRDG),
    m_index(index),
    m_queueType(queueType),
    m_name(name)
{
}
RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    if(m_renderPassMap.contains(name))
    {
        return m_passes[m_renderPassMap[name]];
    }

    auto  index = m_passes.size();
    auto* pass  = m_resourcePool.renderPass.allocate(this, index, queueType, name);
    m_passes.emplace_back(pass);
    m_renderPassMap[name.data()] = index;
    return pass;
}

RenderGraph::RenderGraph(vk::Device* pDevice) : m_pDevice(pDevice)
{
}
void RenderGraph::execute(vk::Image* pImage, vk::SwapChain* pSwapChain)
{
    auto& timer = Timer::GetInstance();
    timer.set("renderer: begin frame");

    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    m_pRenderTarget = pImage;

    vk::QueueSubmitInfo frameSubmitInfo{};

    auto&      taskMgr = TaskManager::GetInstance();
    auto       taskgrp = taskMgr.createTaskGroup();
    std::mutex submitLock;
    for(auto* pass : m_passes)
    {
        taskgrp->addTask(
            [this, pass, queue, &frameSubmitInfo, &submitLock]() {
                auto& cmdPool = pass->m_res.pCmdPools;
                if(cmdPool == nullptr)
                {
                    cmdPool = m_pDevice->acquireCommandPool({queue, false});
                }
                vk::CommandBuffer* pCmd = cmdPool->allocate();
                pCmd->begin();
                pCmd->setDebugName(pass->m_name);
                pCmd->insertDebugLabel({.name = pass->m_name, .color = {0.6f, 0.6f, 0.6f, 0.6f}});
                pCmd->beginRendering(pass->m_res.colorOut);
                pass->m_executeCB(pCmd);
                pCmd->endRendering();
                pCmd->end();

                // lock
                std::lock_guard<std::mutex> holder{submitLock};
                frameSubmitInfo.commandBuffers.push_back(pCmd);
            },
            pass->m_name);
    }
    taskMgr.submit(taskgrp);

    // submission
    {
        vk::Fence* frameFence = m_pDevice->acquireFence();
        frameFence->reset();

        vk::Semaphore* renderSem  = {};
        vk::Semaphore* presentSem = {};

        if(pSwapChain)
        {
            renderSem = m_pDevice->acquireSemaphore();
            APH_CHECK_RESULT(pSwapChain->acquireNextImage(renderSem->getHandle()));

            presentSem = m_pDevice->acquireSemaphore();
            frameSubmitInfo.waitSemaphores.push_back(renderSem);
            frameSubmitInfo.signalSemaphores.push_back(presentSem);
        }

        taskMgr.wait();
        APH_CHECK_RESULT(queue->submit({frameSubmitInfo}, frameFence));

        if(pSwapChain)
        {
            auto pSwapchainImage = pSwapChain->getImage();

            // transisiton && copy
            m_pDevice->executeSingleCommands(queue, [pImage, pSwapchainImage](vk::CommandBuffer* pCopyCmd) {
                pCopyCmd->transitionImageLayout(pImage, RESOURCE_STATE_COPY_SRC);
                pCopyCmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_COPY_DST);

                if(pImage->getWidth() == pSwapchainImage->getWidth() &&
                   pImage->getHeight() == pSwapchainImage->getHeight() &&
                   pImage->getDepth() == pSwapchainImage->getDepth())
                {
                    VK_LOG_DEBUG("copy image to swapchain.");
                    pCopyCmd->copyImage(pImage, pSwapchainImage);
                }
                else
                {
                    VK_LOG_DEBUG("blit image to swapchain.");
                    pCopyCmd->blitImage(pImage, pSwapchainImage);
                }

                pCopyCmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_PRESENT);
            });
            APH_CHECK_RESULT(pSwapChain->presentImage(queue, {presentSem}));
        }

        frameFence->wait();
    }

    timer.set("renderer: end frame");
    m_frameData.frameTime = timer.interval("renderer: begin frame", "renderer: end frame");
    m_frameData.fps       = 1 / m_frameData.frameTime;
    CM_LOG_DEBUG("Fps: %.0f", m_frameData.fps);
}
void RenderPass::addColorOutput(vk::Image* pImage)
{
    if(!m_res.colorOutMap.contains(pImage))
    {
        m_res.colorOut.push_back(pImage);
        m_res.colorOutMap.insert(pImage);
    }
}
}  // namespace aph
