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

PassImageResource* RenderPass::addColorOutput(const std::string& name, const PassImageInfo& info)
{
    if(m_pRenderGraph->hasResource(name))
    {
        auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
        return res;
    }
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_res.colorOutMap.push_back(res);
    return res;
}

PassImageResource* RenderPass::setDepthStencilOutput(const std::string& name, const PassImageInfo& info)
{
    if(m_pRenderGraph->hasResource(name))
    {
        auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
        return res;
    }
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_res.depthOut = res;
    return res;
}

RenderPass* RenderGraph::getPass(const std::string& name)
{
    if(m_renderPassMap.contains(name))
    {
        return m_passes[m_renderPassMap[name]];
    }
    return nullptr;
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

void RenderGraph::build(const std::string& output)
{
    for(auto* pass : m_passes)
    {
        for(auto colorAttachment : pass->m_res.colorOutMap)
        {
            if(!m_buildRes.image.contains(colorAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = colorAttachment->getInfo().extent,
                    .usage     = colorAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = colorAttachment->getInfo().format,
                };
                if(!output.empty() && m_passResourceMap.contains(output))
                {
                    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }
                APH_CHECK_RESULT(m_pDevice->create(createInfo, &pImage));
                m_buildRes.image[colorAttachment] = pImage;
            }
        }

        {
            auto depthAttachment = pass->m_res.depthOut;
            if(depthAttachment && !m_buildRes.image.contains(depthAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = depthAttachment->getInfo().extent,
                    .usage     = depthAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = depthAttachment->getInfo().format,
                };
                APH_CHECK_RESULT(m_pDevice->create(createInfo, &pImage));
                m_buildRes.image[depthAttachment] = pImage;
            }
        }
    }
}

void RenderGraph::execute(const std::string& output, vk::SwapChain* pSwapChain)
{
    auto& timer = Timer::GetInstance();
    timer.set("renderer: begin frame");

    auto fence = executeAsync(output, pSwapChain);
    fence->wait();

    timer.set("renderer: end frame");
    m_frameData.frameTime = timer.interval("renderer: begin frame", "renderer: end frame");
    m_frameData.fps       = 1 / m_frameData.frameTime;
    CM_LOG_DEBUG("Fps: %.0f", m_frameData.fps);
}

RenderGraph::~RenderGraph()
{
    for(auto [_, image] : m_buildRes.image)
    {
        m_pDevice->destroy(image);
    }
}
PassResource* RenderGraph::getResource(const std::string& name, PassResource::Type type)
{
    if(m_passResourceMap.contains(name))
    {
        auto res = m_passResources.at(m_passResourceMap[name]);
        APH_ASSERT(res->getType() == type);
        return res;
    }

    std::size_t   idx = m_passResources.size();
    PassResource* res = {};
    switch(type)
    {
    case PassResource::Type::Image:
        res = m_resourcePool.passImageResource.allocate(type);
        break;
    case PassResource::Type::Buffer:
        res = m_resourcePool.passBufferResource.allocate(type);
        break;
    }

    APH_ASSERT(res);

    m_passResources.emplace_back(res);
    m_passResourceMap[name] = idx;
    return res;
}

vk::Fence* RenderGraph::executeAsync(const std::string& output, vk::SwapChain* pSwapChain)
{
    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    vk::QueueSubmitInfo frameSubmitInfo{};

    auto&      taskMgr = m_taskManager;
    auto       taskgrp = taskMgr.createTaskGroup();
    std::mutex submitLock;

    build(output);

    for(auto* pass : m_passes)
    {
        std::vector<vk::Image*> colorImages;
        vk::Image*              pDepthImage = {};

        colorImages.reserve(pass->m_res.colorOutMap.size());
        for(auto colorAttachment : pass->m_res.colorOutMap)
        {
            colorImages.push_back(m_buildRes.image[colorAttachment]);
        }
        if(pass->m_res.depthOut)
        {
            pDepthImage = m_buildRes.image[pass->m_res.depthOut];
        }

        APH_ASSERT(!colorImages.empty());

        taskgrp->addTask(
            [this, pass, queue, &frameSubmitInfo, &submitLock, colorImages, pDepthImage]() {
                auto& cmdPool = pass->m_res.pCmdPools;
                if(cmdPool == nullptr)
                {
                    cmdPool = m_pDevice->acquireCommandPool({queue, false});
                }
                vk::CommandBuffer* pCmd = cmdPool->allocate();
                pCmd->begin();
                pCmd->setDebugName(pass->m_name);
                pCmd->insertDebugLabel({.name = pass->m_name, .color = {0.6f, 0.6f, 0.6f, 0.6f}});
                pCmd->beginRendering(colorImages, pDepthImage);
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

            auto pImage = m_buildRes.image[m_passResources[m_passResourceMap[output]]];
            // transisiton && copy
            m_pDevice->executeSingleCommands(queue, [pImage, pSwapchainImage](auto* pCopyCmd) {
                pCopyCmd->transitionImageLayout(pImage, ResourceState::CopySource);
                pCopyCmd->transitionImageLayout(pSwapchainImage, ResourceState::CopyDest);

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

                pCopyCmd->transitionImageLayout(pSwapchainImage, ResourceState::Present);
            });
            APH_CHECK_RESULT(pSwapChain->presentImage(queue, {presentSem}));
        }

        return frameFence;
    }
}
}  // namespace aph
