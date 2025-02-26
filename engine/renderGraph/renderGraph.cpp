#include "renderGraph.h"
#include "common/profiler.h"
#include "threads/taskManager.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name) :
    m_pRenderGraph(pRDG),
    m_index(index),
    m_queueType(queueType),
    m_name(name)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pRDG);
}

PassBufferResource* RenderPass::addStorageBufferIn(const std::string& name, vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    res->addUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferIn.push_back(res);

    if(pBuffer)
    {
        m_pRenderGraph->importResource(name, pBuffer);
    }

    return res;
}

PassBufferResource* RenderPass::addUniformBufferIn(const std::string& name, vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    res->addUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::UniformBuffer;
    m_res.uniformBufferIn.push_back(res);

    if(pBuffer)
    {
        m_pRenderGraph->importResource(name, pBuffer);
    }

    return res;
}

PassBufferResource* RenderPass::addBufferOut(const std::string& name)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addWritePass(this);
    res->addUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_WRITE_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferOut.push_back(res);

    return res;
}
PassImageResource* RenderPass::addTextureOut(const std::string& name)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_STORAGE_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.textureOut.push_back(res);

    return res;
}

PassImageResource* RenderPass::addTextureIn(const std::string& name, vk::Image* pImage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addReadPass(this);
    res->addUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::ShaderResource;
    m_res.textureIn.push_back(res);

    if(pImage)
    {
        m_pRenderGraph->importResource(name, pImage);
    }

    return res;
}

PassImageResource* RenderPass::setColorOut(const std::string& name, const PassImageInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_res.resourceStateMap[res] = ResourceState::RenderTarget;
    m_res.colorOut.push_back(res);
    return res;
}

PassImageResource* RenderPass::setDepthStencilOut(const std::string& name, const PassImageInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_res.resourceStateMap[res] = ResourceState::DepthStencil;
    m_res.depthOut              = res;
    return res;
}

RenderPass* RenderGraph::getPass(const std::string& name)
{
    APH_PROFILER_SCOPE();
    if(m_declareData.passMap.contains(name))
    {
        return m_declareData.passes[m_declareData.passMap[name]];
    }
    return nullptr;
}
RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    APH_PROFILER_SCOPE();
    if(m_declareData.passMap.contains(name))
    {
        return m_declareData.passes[m_declareData.passMap[name]];
    }

    auto  index = m_declareData.passes.size();
    auto* pass  = m_resourcePool.renderPass.allocate(this, index, queueType, name);
    m_declareData.passes.emplace_back(pass);
    m_declareData.passMap[name] = index;
    return pass;
}

RenderGraph::RenderGraph(vk::Device* pDevice) : m_pDevice(pDevice)
{
}

void RenderGraph::build(vk::SwapChain* pSwapChain)
{
    APH_PROFILER_SCOPE();
    // TODO clear on demand
    {
        m_buildData.bufferBarriers.clear();
        m_buildData.imageBarriers.clear();
        m_buildData.frameSubmitInfos.clear();
        for(auto* pass : m_declareData.passes)
        {
            m_buildData.passDependencyGraph[pass].clear();
        }
    }

    if(m_buildData.frameFence == nullptr)
    {
        m_buildData.frameFence = m_pDevice->acquireFence(false);
    }

    if(m_buildData.presentSem == nullptr)
    {
        m_buildData.presentSem = m_pDevice->acquireSemaphore();
    }

    if(m_buildData.renderSem == nullptr)
    {
        m_buildData.renderSem = m_pDevice->acquireSemaphore();
    }

    for(auto res : m_declareData.resources)
    {
        for(const auto& readPass : res->getReadPasses())
        {
            for(const auto& writePass : res->getWritePasses())
            {
                if(readPass != writePass)
                {
                    m_buildData.passDependencyGraph[readPass].insert(writePass);
                }
            }
        }
    }

    // topological sort
    {
        if(m_buildData.passDependencyGraph.empty())
        {
            VK_LOG_WARN("render graph is empty.");
        }
        std::unordered_map<RenderPass*, int> inDegree;
        std::queue<RenderPass*>              zeroInDegreeQueue;
        auto&                                result = m_buildData.sortedPasses;
        auto&                                graph  = m_buildData.passDependencyGraph;

        // Initialize in-degree of each node
        for(auto& [pass, nodes] : graph)
        {
            if(!inDegree.contains(pass))
            {
                inDegree[pass] = 0;
            }

            for(RenderPass* node : nodes)
            {
                inDegree[node]++;
            }
        }

        // Find all nodes with in-degree of 0
        for(auto& [pass, degree] : inDegree)
        {
            if(degree == 0)
            {
                zeroInDegreeQueue.push(pass);
            }
        }

        // Topological Sort
        while(!zeroInDegreeQueue.empty())
        {
            RenderPass* node = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            result.push_back(node);

            // Decrease the in-degree of adjacent nodes
            for(RenderPass* adjacent : graph[node])
            {
                --inDegree[adjacent];
                if(inDegree[adjacent] == 0)
                {
                    zeroInDegreeQueue.push(adjacent);
                }
            }
        }

        // Check if there was a cycle in the graph
        APH_ASSERT(result.size() == graph.size());
    }

    // per pass resource build
    for(auto* pass : m_buildData.sortedPasses)
    {
        auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);
        if(!m_buildData.cmdPools.contains(pass))
        {
            APH_VR(m_pDevice->create({queue, false}, &m_buildData.cmdPools[pass]));
            m_buildData.cmds[pass]     = m_buildData.cmdPools[pass]->allocate();
        }

        // color attachments
        for(auto colorAttachment : pass->m_res.colorOut)
        {
            if(!m_buildData.image.contains(colorAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = colorAttachment->getInfo().extent,
                    .usage     = colorAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = colorAttachment->getInfo().format,
                };
                if(!m_declareData.backBuffer.empty() && m_declareData.resourceMap.contains(m_declareData.backBuffer))
                {
                    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }
                APH_VR(m_pDevice->create(createInfo, &pImage));
                m_buildData.image[colorAttachment] = pImage;
            }
        }

        // depth attachments
        {
            auto depthAttachment = pass->m_res.depthOut;
            if(depthAttachment && !m_buildData.image.contains(depthAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = depthAttachment->getInfo().extent,
                    .usage     = depthAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = depthAttachment->getInfo().format,
                };
                APH_VR(m_pDevice->create(createInfo, &pImage));
                m_buildData.image[depthAttachment] = pImage;
            }
        }
    }

    // swapchain
    {
        m_buildData.pSwapchain = pSwapChain;
    }

    // record commands
    {
        auto& taskMgr = m_taskManager;
        auto  taskgrp = taskMgr.createTaskGroup();

        for(auto* pass : m_declareData.passes)
        {
            std::vector<vk::Image*>         colorImages;
            vk::Image*                      pDepthImage = {};
            std::vector<vk::ImageBarrier>   initImageBarriers{};
            std::vector<vk::ImageBarrier>&  imageBarriers  = m_buildData.imageBarriers[pass];
            std::vector<vk::BufferBarrier>& bufferBarriers = m_buildData.bufferBarriers[pass];

            colorImages.reserve(pass->m_res.colorOut.size());
            for(PassImageResource* colorAttachment : pass->m_res.colorOut)
            {
                colorImages.push_back(m_buildData.image[colorAttachment]);
                auto& image = colorImages.back();
                initImageBarriers.push_back({
                    .pImage       = image,
                    .currentState = ResourceState::Undefined,
                    .newState     = ResourceState::RenderTarget,
                });
            }
            if(pass->m_res.depthOut)
            {
                pDepthImage = m_buildData.image[pass->m_res.depthOut];
                initImageBarriers.push_back({
                    .pImage       = pDepthImage,
                    .currentState = ResourceState::Undefined,
                    .newState     = ResourceState::DepthStencil,
                });
            }

            auto queue = m_pDevice->getQueue(QueueType::Graphics);
            m_pDevice->executeCommand(
                queue, [&initImageBarriers](auto* pCmd) { pCmd->insertBarrier(initImageBarriers); });

            for(PassImageResource* textureIn : pass->m_res.textureIn)
            {
                auto& image = m_buildData.image[textureIn];
                if(image->getResourceState() != pass->m_res.resourceStateMap[textureIn])
                {
                    imageBarriers.push_back({
                        .pImage       = image,
                        .currentState = image->getResourceState(),
                        .newState     = pass->m_res.resourceStateMap[textureIn],
                    });
                }
            }

            for(PassBufferResource* bufferIn : pass->m_res.storageBufferIn)
            {
                auto& buffer = m_buildData.buffer[bufferIn];
                if(buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
                {
                    bufferBarriers.push_back(vk::BufferBarrier{
                        .pBuffer      = buffer,
                        .currentState = buffer->getResourceState(),
                        .newState     = pass->m_res.resourceStateMap[bufferIn],
                    });
                }
            }

            for(PassBufferResource* bufferIn : pass->m_res.uniformBufferIn)
            {
                auto& buffer = m_buildData.buffer[bufferIn];
                if(buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
                {
                    bufferBarriers.push_back(vk::BufferBarrier{
                        .pBuffer      = buffer,
                        .currentState = buffer->getResourceState(),
                        .newState     = pass->m_res.resourceStateMap[bufferIn],
                    });
                }
            }

            APH_ASSERT(!colorImages.empty());

            taskgrp->addTask(
                [this, pass, colorImages, pDepthImage]() {
                    auto* pCmd = m_buildData.cmds[pass];
                    pCmd->begin();
                    // TODO findout why memory leaks
                    pCmd->insertDebugLabel({.name = pass->m_name, .color = {0.6f, 0.6f, 0.6f, 0.6f}});
                    pCmd->insertBarrier(m_buildData.bufferBarriers[pass], m_buildData.imageBarriers[pass]);
                    pCmd->beginRendering(colorImages, pDepthImage);
                    APH_ASSERT(pass->m_executeCB);
                    pass->m_executeCB(pCmd);
                    pCmd->endRendering();
                    pCmd->end();

                    // lock
                    vk::QueueSubmitInfo submitInfo{
                        .commandBuffers   = {pCmd},
                        .waitSemaphores   = {},
                        .signalSemaphores = {},
                    };

                    std::lock_guard<std::mutex> holder{m_buildData.submitLock};
                    m_buildData.frameSubmitInfos.push_back(std::move(submitInfo));
                },
                pass->m_name);
        }
        taskMgr.submit(taskgrp);
    }

    m_taskManager.wait();
}

RenderGraph::~RenderGraph()
{
    APH_PROFILER_SCOPE();
    cleanup();
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    auto res = getResource(name, PassResource::Type::Buffer);
    APH_ASSERT(!m_buildData.buffer.contains(res));
    res->addFlags(PASS_RESOURCE_EXTERNAL);
    m_buildData.buffer[res] = pBuffer;
    return res;
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Image* pImage)
{
    APH_PROFILER_SCOPE();
    auto res = getResource(name, PassResource::Type::Image);
    APH_ASSERT(!m_buildData.image.contains(res));
    res->addFlags(PASS_RESOURCE_EXTERNAL);
    m_buildData.image[res] = pImage;
    return res;
}

PassResource* RenderGraph::getResource(const std::string& name, PassResource::Type type)
{
    APH_PROFILER_SCOPE();
    if(m_declareData.resourceMap.contains(name))
    {
        auto res = m_declareData.resources.at(m_declareData.resourceMap[name]);
        APH_ASSERT(res->getType() == type);
        return res;
    }

    std::size_t   idx = m_declareData.resources.size();
    PassResource* res = {};
    switch(type)
    {
    case PassResource::Type::Image:
        res = m_resourcePool.passImageResource.allocate(type);
        m_declareData.imageResources.emplace_back(static_cast<PassImageResource*>(res));
        break;
    case PassResource::Type::Buffer:
        res = m_resourcePool.passBufferResource.allocate(type);
        m_declareData.bufferResources.emplace_back(static_cast<PassBufferResource*>(res));
        break;
    }

    APH_ASSERT(res);

    m_declareData.resources.emplace_back(res);
    m_declareData.resourceMap[name] = idx;
    return res;
}

void RenderGraph::execute(vk::Fence* pFence)
{
    APH_PROFILER_SCOPE();
    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    // submit && present
    {
        vk::Fence* frameFence = pFence ? pFence : m_buildData.frameFence;
        frameFence->reset();

        APH_VR(queue->submit(m_buildData.frameSubmitInfos, frameFence));

        if(m_buildData.pSwapchain)
        {
            APH_VR(m_buildData.pSwapchain->acquireNextImage(m_buildData.renderSem));

            m_pDevice->executeCommand(
                m_pDevice->getQueue(aph::QueueType::Transfer),
                [this](auto* pCopyCmd) {
                    auto pSwapchainImage = m_buildData.pSwapchain->getImage();
                    auto pOutImage =
                        m_buildData.image[m_declareData.resources[m_declareData.resourceMap[m_declareData.backBuffer]]];

                    pCopyCmd->insertBarrier({
                        {
                            .pImage       = pOutImage,
                            .currentState = ResourceState::RenderTarget,
                            .newState     = ResourceState::CopySource,
                        },
                        {
                            .pImage       = pSwapchainImage,
                            .currentState = ResourceState::Undefined,
                            .newState     = ResourceState::CopyDest,
                        },
                    });

                    if(pOutImage->getWidth() == pSwapchainImage->getWidth() &&
                       pOutImage->getHeight() == pSwapchainImage->getHeight() &&
                       pOutImage->getDepth() == pSwapchainImage->getDepth())
                    {
                        VK_LOG_DEBUG("copy image to swapchain.");
                        pCopyCmd->copyImage(pOutImage, pSwapchainImage);
                    }
                    else
                    {
                        VK_LOG_DEBUG("blit image to swapchain.");
                        pCopyCmd->blitImage(pOutImage, pSwapchainImage);
                    }

                    pCopyCmd->insertBarrier({
                        {
                            .pImage       = pOutImage,
                            .currentState = ResourceState::Undefined,
                            .newState     = ResourceState::RenderTarget,
                        },
                        {
                            .pImage       = pSwapchainImage,
                            .currentState = ResourceState::CopyDest,
                            .newState     = ResourceState::Present,
                        },
                    });
                },
                {m_buildData.renderSem}, {m_buildData.presentSem});
        }

        APH_VR(m_buildData.pSwapchain->presentImage({m_buildData.presentSem}));

        if(!pFence)
        {
            frameFence->wait();
            APH_VR(m_pDevice->releaseFence(frameFence));
        }
    }
}

vk::Image* RenderGraph::getBuildResource(PassImageResource* pResource) const
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_buildData.image.contains(pResource));
    return m_buildData.image.at(pResource);
}
vk::Buffer* RenderGraph::getBuildResource(PassBufferResource* pResource) const
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_buildData.buffer.contains(pResource));
    return m_buildData.buffer.at(pResource);
}
void RenderGraph::setBackBuffer(const std::string& backBuffer)
{
    APH_PROFILER_SCOPE();
    m_declareData.backBuffer = backBuffer;
}

void RenderGraph::cleanup()
{
    {
        m_buildData.bufferBarriers.clear();
        m_buildData.imageBarriers.clear();
        m_buildData.frameSubmitInfos.clear();
        for(auto* pass : m_declareData.passes)
        {
            m_buildData.passDependencyGraph[pass].clear();
        }

        for(auto pass: m_declareData.passes)
        {
            m_resourcePool.renderPass.free(pass);
        }
        m_declareData.passes.clear();

        for(auto resource: m_declareData.imageResources)
        {
            m_resourcePool.passImageResource.free(resource);
        }
        m_declareData.imageResources.clear();

        for(auto resource: m_declareData.bufferResources)
        {
            m_resourcePool.passBufferResource.free(resource);
        }
        m_declareData.bufferResources.clear();
    }

    for (auto [_, cmdPool]: m_buildData.cmdPools)
    {
        m_pDevice->destroy(cmdPool);
    }
    m_buildData.cmdPools.clear();

    for(auto* res : m_declareData.resources)
    {
        if(!(res->getFlags() & PASS_RESOURCE_EXTERNAL))
        {
            auto pImage = m_buildData.image[res];
            m_pDevice->destroy(pImage);
        }
    }

};
}  // namespace aph
