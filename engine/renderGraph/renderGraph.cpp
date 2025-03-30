#include "renderGraph.h"
#include "common/graphView.h"
#include "common/profiler.h"
#include "threads/taskManager.h"

namespace aph
{
RenderGraph::RenderGraph(vk::Device* pDevice)
    : m_pDevice(pDevice)
{
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
    auto res = getPassResource(name, PassResource::Type::Buffer);
    APH_ASSERT(!m_buildData.buffer.contains(res));
    res->addFlags(PassResourceFlagBits::External);
    m_buildData.buffer[res] = pBuffer;
    return res;
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Image* pImage)
{
    APH_PROFILER_SCOPE();
    auto res = getPassResource(name, PassResource::Type::Image);
    APH_ASSERT(!m_buildData.image.contains(res));
    res->addFlags(PassResourceFlagBits::External);
    m_buildData.image[res] = pImage;
    return res;
}

PassResource* RenderGraph::getPassResource(const std::string& name, PassResource::Type type)
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

std::string RenderGraph::exportToGraphviz() const
{
    APH_PROFILER_SCOPE();

    // Create a graph visualizer instance
    GraphVisualizer visualizer;

    // Configure the graph settings
    visualizer.setName("RenderGraph");
    visualizer.setDirection(GraphDirection::LeftToRight);
    visualizer.setFontName("Arial");
    visualizer.setNodeSeparation(0.8f);
    visualizer.setRankSeparation(1.0f);

    // Define default styles
    GraphColor nodeGraphicsFill = GraphColor::fromHex("#A3D977");
    GraphColor nodeGraphicsBorder = GraphColor::fromHex("#2D6016");
    GraphColor nodeComputeFill = GraphColor::fromHex("#7891D0");
    GraphColor nodeComputeBorder = GraphColor::fromHex("#1A337E");
    GraphColor nodeTransferFill = GraphColor::fromHex("#E8C477");
    GraphColor nodeTransferBorder = GraphColor::fromHex("#8E6516");
    GraphColor nodeDefaultFill = GraphColor::fromHex("#D3D3D3");
    GraphColor nodeDefaultBorder = GraphColor::fromHex("#5A5A5A");

    GraphColor edgeImageColor = GraphColor::fromHex("#4285F4");
    GraphColor edgeBufferColor = GraphColor::fromHex("#EA4335");

    // Add nodes (passes)
    for (const auto& [name, pass] : m_declareData.passMap)
    {
        GraphNode* node = visualizer.addNode(name);

        // Set color based on queue type
        switch (pass->getQueueType())
        {
        case QueueType::Graphics:
            node->setFillColor(nodeGraphicsFill);
            node->setBorderColor(nodeGraphicsBorder);
            break;
        case QueueType::Compute:
            node->setFillColor(nodeComputeFill);
            node->setBorderColor(nodeComputeBorder);
            break;
        case QueueType::Transfer:
            node->setFillColor(nodeTransferFill);
            node->setBorderColor(nodeTransferBorder);
            break;
        default:
            node->setFillColor(nodeDefaultFill);
            node->setBorderColor(nodeDefaultBorder);
        }

        // Create HTML-like table for the node content
        node->beginTable();

        // Add pass name as header
        node->addTableRow(name, "", true);

        // Add queue type
        std::string queueTypeStr;
        switch (pass->getQueueType())
        {
        case QueueType::Graphics:
            queueTypeStr = "Graphics";
            break;
        case QueueType::Compute:
            queueTypeStr = "Compute";
            break;
        case QueueType::Transfer:
            queueTypeStr = "Transfer";
            break;
        default:
            queueTypeStr = "Unknown";
        }
        node->addTableRow("Queue:", queueTypeStr);

        // Add resource inputs
        if (!pass->m_res.textureIn.empty() || !pass->m_res.uniformBufferIn.empty() ||
            !pass->m_res.storageBufferIn.empty())
        {

            std::string inputs;
            bool first = true;

            for (const auto& resource : pass->m_res.textureIn)
            {
                inputs += (first ? "" : "<BR/>") + std::string("Texture: ") + resource->getName();
                first = false;
            }
            for (const auto& resource : pass->m_res.uniformBufferIn)
            {
                inputs += (first ? "" : "<BR/>") + std::string("Uniform: ") + resource->getName();
                first = false;
            }
            for (const auto& resource : pass->m_res.storageBufferIn)
            {
                inputs += (first ? "" : "<BR/>") + std::string("Storage: ") + resource->getName();
                first = false;
            }

            node->addTableRow("Inputs:", inputs);
        }

        // Add resource outputs
        if (!pass->m_res.textureOut.empty() || !pass->m_res.storageBufferOut.empty() || !pass->m_res.colorOut.empty() ||
            pass->m_res.depthOut)
        {

            std::string outputs;
            bool first = true;

            for (const auto& resource : pass->m_res.textureOut)
            {
                outputs += (first ? "" : "<BR/>") + std::string("Texture: ") + resource->getName();
                first = false;
            }
            for (const auto& resource : pass->m_res.storageBufferOut)
            {
                outputs += (first ? "" : "<BR/>") + std::string("Storage: ") + resource->getName();
                first = false;
            }
            for (const auto& resource : pass->m_res.colorOut)
            {
                outputs += (first ? "" : "<BR/>") + std::string("Color: ") + resource->getName();
                first = false;
            }
            if (pass->m_res.depthOut)
            {
                outputs += (first ? "" : "<BR/>") + std::string("Depth: ") + pass->m_res.depthOut->getName();
            }

            node->addTableRow("Outputs:", outputs);
        }

        node->endTable();
    }

    // Add edges based on resource dependencies
    for (const auto& [name, resource] : m_declareData.resourceMap)
    {
        // For each resource, draw edges from write passes to read passes
        for (const auto& writePass : resource->getWritePasses())
        {
            for (const auto& readPass : resource->getReadPasses())
            {
                if (writePass != readPass)
                {
                    // Create edge
                    GraphEdge* edge = visualizer.addEdge(writePass->m_name, readPass->m_name);
                    edge->setLabel(name);

                    // Style based on resource type
                    if (resource->getType() == PassResource::Type::Image)
                    {
                        edge->setColor(edgeImageColor);
                    }
                    else
                    {
                        edge->setColor(edgeBufferColor);
                    }
                    edge->setThickness(1.5f);
                }
            }
        }
    }

    // Export to DOT format
    return visualizer.exportToDot();
}

PassResource* RenderGraph::findPassResource(const std::string& name) const
{
    APH_PROFILER_SCOPE();
    if (auto it = m_declareData.resourceMap.find(name); it != m_declareData.resourceMap.end())
    {
        return it->second;
    }
    return nullptr;
}

template <>
vk::Image* RenderGraph::getResource<vk::Image>(const std::string& name)
{
    if (auto* resource = findPassResource(name); resource && resource->getType() == PassResource::Type::Image)
    {
        if (auto it = m_buildData.image.find(resource); it != m_buildData.image.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

template <>
vk::Buffer* RenderGraph::getResource<vk::Buffer>(const std::string& name)
{
    if (auto* resource = findPassResource(name); resource && resource->getType() == PassResource::Type::Buffer)
    {
        if (auto it = m_buildData.buffer.find(resource); it != m_buildData.buffer.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

} // namespace aph
