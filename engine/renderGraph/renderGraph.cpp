#include "renderGraph.h"
#include "common/profiler.h"
#include "threads/taskManager.h"

namespace aph
{
RenderGraph::RenderGraph(vk::Device* pDevice)
    : m_pDevice(pDevice)
{
}

RenderPass* RenderGraph::getPass(const std::string& name)
{
    APH_PROFILER_SCOPE();
    if (m_declareData.passMap.contains(name))
    {
        return m_declareData.passMap[name];
    }
    return nullptr;
}

RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    APH_PROFILER_SCOPE();
    if (m_declareData.passMap.contains(name))
    {
        return m_declareData.passMap[name];
    }

    auto* pass = m_resourcePool.renderPass.allocate(this, queueType, name);
    m_declareData.passMap[name] = pass;
    return pass;
}

void RenderGraph::build(vk::SwapChain* pSwapChain)
{
    APH_PROFILER_SCOPE();

    if (pSwapChain != m_buildData.pSwapchain)
    {
        m_buildData.pSwapchain = pSwapChain;
        setDirty(DirtyFlagBits::SwapChainDirty);
    }

    // If nothing is dirty, no need to rebuild
    if (m_dirtyFlags == DirtyFlagBits::None)
    {
        return;
    }

    // Clear relevant data structures based on what's dirty
    if (isDirty(DirtyFlagBits::TopologyDirty | DirtyFlagBits::PassDirty))
    {
        std::lock_guard<std::mutex> holder{ m_buildData.submitLock };
        m_buildData.bufferBarriers.clear();
        m_buildData.imageBarriers.clear();
        m_buildData.frameSubmitInfos.clear();
        m_buildData.sortedPasses.clear();

        for (auto [name, pass] : m_declareData.passMap)
        {
            m_buildData.passDependencyGraph[pass].clear();
        }
    }

    if (isDirty(DirtyFlagBits::TopologyDirty | DirtyFlagBits::PassDirty))
    {
        APH_PROFILER_SCOPE_NAME("topological sort");

        for (auto [name, res] : m_declareData.resourceMap)
        {
            for (const auto& readPass : res->getReadPasses())
            {
                for (const auto& writePass : res->getWritePasses())
                {
                    if (readPass != writePass)
                    {
                        m_buildData.passDependencyGraph[readPass].insert(writePass);
                    }
                }
            }
        }

        if (m_buildData.passDependencyGraph.empty())
        {
            VK_LOG_WARN("render graph is empty.");
        }

        HashMap<RenderPass*, int> inDegree;
        std::queue<RenderPass*> zeroInDegreeQueue;
        auto& sortedPasses = m_buildData.sortedPasses;
        auto& graph = m_buildData.passDependencyGraph;

        // Initialize in-degree of each node
        for (auto& [pass, nodes] : graph)
        {
            if (!inDegree.contains(pass))
            {
                inDegree[pass] = 0;
            }

            for (RenderPass* node : nodes)
            {
                inDegree[node]++;
            }
        }

        // Find all nodes with in-degree of 0
        for (auto& [pass, degree] : inDegree)
        {
            if (degree == 0)
            {
                zeroInDegreeQueue.push(pass);
            }
        }

        // Topological Sort
        while (!zeroInDegreeQueue.empty())
        {
            RenderPass* node = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            sortedPasses.push_back(node);

            // Decrease the in-degree of adjacent nodes
            for (RenderPass* adjacent : graph[node])
            {
                --inDegree[adjacent];
                if (inDegree[adjacent] == 0)
                {
                    zeroInDegreeQueue.push(adjacent);
                }
            }
        }

        // Check if there was a cycle in the graph
        APH_ASSERT(sortedPasses.size() == graph.size());
    }

    if (isDirty(DirtyFlagBits::ImageResourceDirty | DirtyFlagBits::BufferResourceDirty | DirtyFlagBits::PassDirty |
                DirtyFlagBits::BackBufferDirty))
    {
        // per pass resource build
        for (auto* pass : m_buildData.sortedPasses)
        {
            APH_PROFILER_SCOPE_NAME("pass resource build");
            auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

            // Create command pools and allocate command buffers if needed
            if (!m_buildData.cmdPools.contains(pass))
            {
                APH_VR(m_pDevice->create(vk::CommandPoolCreateInfo{ queue, false }, &m_buildData.cmdPools[pass]));
                m_buildData.cmds[pass] = m_buildData.cmdPools[pass]->allocate();
            }

            // Create or update color attachments
            for (PassImageResource* colorAttachment : pass->m_res.colorOut)
            {
                bool needsRebuild = !m_buildData.image.contains(colorAttachment);

                // Rebuild if image resource is dirty and this resource isn't external
                if (isDirty(DirtyFlagBits::ImageResourceDirty) &&
                    !(colorAttachment->getFlags() & PassResourceFlagBits::External))
                {
                    needsRebuild = true;
                }

                if (needsRebuild)
                {
                    // Clean up previous image if one exists
                    if (m_buildData.image.contains(colorAttachment) &&
                        !(colorAttachment->getFlags() & PassResourceFlagBits::External))
                    {
                        m_pDevice->destroy(m_buildData.image[colorAttachment]);
                    }

                    vk::Image* pImage = {};
                    vk::ImageCreateInfo createInfo{
                        .extent = colorAttachment->getInfo().extent,
                        .usage = colorAttachment->getUsage(),
                        .domain = MemoryDomain::Device,
                        .imageType = ImageType::e2D,
                        .format = colorAttachment->getInfo().format,
                    };

                    if (!m_declareData.backBuffer.empty() &&
                        m_declareData.resourceMap.contains(m_declareData.backBuffer))
                    {
                        createInfo.usage |= ImageUsage::TransferSrc;
                    }

                    APH_VR(m_pDevice->create(createInfo, &pImage, colorAttachment->getName()));
                    m_buildData.image[colorAttachment] = pImage;
                }
            }

            // Create or update depth attachments
            {
                auto depthAttachment = pass->m_res.depthOut;
                if (depthAttachment)
                {
                    bool needsRebuild = !m_buildData.image.contains(depthAttachment);

                    // Rebuild if image resource is dirty and this resource isn't external
                    if (isDirty(DirtyFlagBits::ImageResourceDirty) &&
                        !(depthAttachment->getFlags() & PassResourceFlagBits::External))
                    {
                        needsRebuild = true;
                    }

                    if (needsRebuild)
                    {
                        // Clean up previous image if one exists
                        if (m_buildData.image.contains(depthAttachment) &&
                            !(depthAttachment->getFlags() & PassResourceFlagBits::External))
                        {
                            m_pDevice->destroy(m_buildData.image[depthAttachment]);
                        }

                        vk::Image* pImage = {};
                        vk::ImageCreateInfo createInfo{
                            .extent = depthAttachment->getInfo().extent,
                            .usage = depthAttachment->getUsage(),
                            .domain = MemoryDomain::Device,
                            .imageType = ImageType::e2D,
                            .format = depthAttachment->getInfo().format,
                        };

                        APH_VR(m_pDevice->create(createInfo, &pImage, depthAttachment->getName()));
                        m_buildData.image[depthAttachment] = pImage;
                    }
                }
            }
        }
    }

    // Record commands for each pass
    if (isDirty(DirtyFlagBits::PassDirty | DirtyFlagBits::ImageResourceDirty | DirtyFlagBits::BufferResourceDirty |
                DirtyFlagBits::TopologyDirty))
    {
        for (auto [name, pass] : m_declareData.passMap)
        {
            APH_PROFILER_SCOPE_NAME("pass commands recording");
            SmallVector<vk::Image*> colorImages;
            vk::Image* pDepthImage = {};
            SmallVector<vk::ImageBarrier> initImageBarriers{};
            SmallVector<vk::ImageBarrier>& imageBarriers = m_buildData.imageBarriers[pass];
            SmallVector<vk::BufferBarrier>& bufferBarriers = m_buildData.bufferBarriers[pass];

            // Clear existing barriers
            imageBarriers.clear();
            bufferBarriers.clear();

            // Collect color images
            colorImages.reserve(pass->m_res.colorOut.size());
            for (PassImageResource* colorAttachment : pass->m_res.colorOut)
            {
                colorImages.push_back(m_buildData.image[colorAttachment]);
                auto& image = colorImages.back();
                initImageBarriers.push_back({
                    .pImage = image,
                    .currentState = ResourceState::Undefined,
                    .newState = ResourceState::RenderTarget,
                });
            }

            // Set up depth image
            if (pass->m_res.depthOut)
            {
                pDepthImage = m_buildData.image[pass->m_res.depthOut];
                initImageBarriers.push_back({
                    .pImage = pDepthImage,
                    .currentState = ResourceState::Undefined,
                    .newState = ResourceState::DepthStencil,
                });
            }

            auto queue = m_pDevice->getQueue(QueueType::Graphics);
            m_pDevice->executeCommand(queue,
                                      [&initImageBarriers](auto* pCmd) { pCmd->insertBarrier(initImageBarriers); });

            // Set up texture barriers
            for (PassImageResource* textureIn : pass->m_res.textureIn)
            {
                auto& image = m_buildData.image[textureIn];
                if (image->getResourceState() != pass->m_res.resourceStateMap[textureIn])
                {
                    imageBarriers.push_back({
                        .pImage = image,
                        .currentState = image->getResourceState(),
                        .newState = pass->m_res.resourceStateMap[textureIn],
                    });
                }
            }

            // Set up storage buffer barriers
            for (PassBufferResource* bufferIn : pass->m_res.storageBufferIn)
            {
                auto& buffer = m_buildData.buffer[bufferIn];
                if (buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
                {
                    bufferBarriers.push_back(vk::BufferBarrier{
                        .pBuffer = buffer,
                        .currentState = buffer->getResourceState(),
                        .newState = pass->m_res.resourceStateMap[bufferIn],
                    });
                }
            }

            // Set up uniform buffer barriers
            for (PassBufferResource* bufferIn : pass->m_res.uniformBufferIn)
            {
                auto& buffer = m_buildData.buffer[bufferIn];
                if (buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
                {
                    bufferBarriers.push_back(vk::BufferBarrier{
                        .pBuffer = buffer,
                        .currentState = buffer->getResourceState(),
                        .newState = pass->m_res.resourceStateMap[bufferIn],
                    });
                }
            }

            APH_ASSERT(!colorImages.empty());

            // Record and submit commands
            {
                APH_PROFILER_SCOPE_NAME("pass commands submit");
                auto* pCmd = m_buildData.cmds[pass];
                APH_VR(pCmd->begin());
                pCmd->insertDebugLabel({ .name = pass->m_name, .color = { 0.6f, 0.6f, 0.6f, 0.6f } });
                pCmd->insertBarrier(m_buildData.bufferBarriers[pass], m_buildData.imageBarriers[pass]);
                pCmd->beginRendering(colorImages, pDepthImage);
                APH_ASSERT(pass->m_executeCB);
                pass->m_executeCB(pCmd);
                pCmd->endRendering();
                APH_VR(pCmd->end());

                vk::QueueSubmitInfo submitInfo{
                    .commandBuffers = { pCmd },
                    .waitSemaphores = {},
                    .signalSemaphores = {},
                };

                std::lock_guard<std::mutex> holder{ m_buildData.submitLock };
                m_buildData.frameSubmitInfos.push_back(std::move(submitInfo));
            }
        }
    }

    // All dirty flags have been handled
    clearDirtyFlags();
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
    res->addFlags(PassResourceFlagBits::External);
    m_buildData.buffer[res] = pBuffer;
    return res;
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Image* pImage)
{
    APH_PROFILER_SCOPE();
    auto res = getResource(name, PassResource::Type::Image);
    APH_ASSERT(!m_buildData.image.contains(res));
    res->addFlags(PassResourceFlagBits::External);
    m_buildData.image[res] = pImage;
    return res;
}

PassResource* RenderGraph::getResource(const std::string& name, PassResource::Type type)
{
    APH_PROFILER_SCOPE();
    if (m_declareData.resourceMap.contains(name))
    {
        auto res = m_declareData.resourceMap[name];
        APH_ASSERT(res->getType() == type);
        return res;
    }

    PassResource* res = {};
    switch (type)
    {
    case PassResource::Type::Image:
        res = m_resourcePool.passImageResource.allocate(type);
        break;
    case PassResource::Type::Buffer:
        res = m_resourcePool.passBufferResource.allocate(type);
        break;
    }
    res->setName(name);

    APH_ASSERT(res);

    m_declareData.resourceMap[name] = res;
    return res;
}

void RenderGraph::execute(vk::Fence* pFence)
{
    APH_PROFILER_SCOPE();
    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    // submit && present
    {
        if (m_buildData.frameFence == nullptr)
        {
            m_buildData.frameFence = m_pDevice->acquireFence(true);
        }

        vk::Fence* frameFence = pFence ? pFence : m_buildData.frameFence;
        {
            frameFence->wait();
            frameFence->reset();
        }

        APH_VR(queue->submit(m_buildData.frameSubmitInfos, frameFence));

        if (m_buildData.pSwapchain)
        {
            auto outImage = m_buildData.image[m_declareData.resourceMap[m_declareData.backBuffer]];
            APH_VR(m_buildData.pSwapchain->presentImage({}, outImage));
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
        for (auto [name, pass] : m_declareData.passMap)
        {
            m_buildData.passDependencyGraph[pass].clear();
            m_resourcePool.renderPass.free(pass);
        }
        m_declareData.passMap.clear();

        for (auto [name, pResource] : m_declareData.resourceMap)
        {
            switch (pResource->getType())
            {
            case PassResource::Type::Image:
            {
                m_resourcePool.passImageResource.free(static_cast<PassImageResource*>(pResource));
                if (!(pResource->getFlags() & PassResourceFlagBits::External))
                {
                    auto pImage = m_buildData.image[pResource];
                    m_pDevice->destroy(pImage);
                }
            }
            break;
            case PassResource::Type::Buffer:
            {
                m_resourcePool.passBufferResource.free(static_cast<PassBufferResource*>(pResource));
                if (!(pResource->getFlags() & PassResourceFlagBits::External))
                {
                    auto pBuffer = m_buildData.buffer[pResource];
                    m_pDevice->destroy(pBuffer);
                }
            }
            break;
            }
        }
        m_declareData.resourceMap.clear();
    }

    for (auto [_, cmdPool] : m_buildData.cmdPools)
    {
        m_pDevice->destroy(cmdPool);
    }
    m_buildData.cmdPools.clear();
};
} // namespace aph
