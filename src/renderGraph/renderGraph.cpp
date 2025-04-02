#include "renderGraph.h"

#include "common/graphView.h"
#include "common/profiler.h"

#include "threads/taskManager.h"

namespace aph
{
// Constructor for normal GPU mode
RenderGraph::RenderGraph(vk::Device* pDevice)
    : m_pDevice(pDevice)
{
    // Create a fence for frame synchronization
    m_buildData.frameExecuteFence = m_pDevice->acquireFence(true);
    m_pCommandBufferAllocator = m_pDevice->getCommandBufferAllocator();
}

// Constructor for dry run mode (no GPU operations)
RenderGraph::RenderGraph()
    : m_pDevice(nullptr)
    , m_debugOutputEnabled(true)
{
    if (m_debugOutputEnabled)
    {
        RDG_LOG_INFO("[DryRun] Created RenderGraph in dry run mode (no GPU operations)");
    }
}

RenderGraph::~RenderGraph()
{
    APH_PROFILER_SCOPE();
    
    // Release any remaining command buffers first
    if (!isDryRunMode() && m_pDevice)
    {
        for (auto [pass, cmdBuffer] : m_buildData.cmds)
        {
            if (cmdBuffer)
            {
                m_pCommandBufferAllocator->release(cmdBuffer);
            }
        }
        m_buildData.cmds.clear();
    }
    
    cleanup();
}

RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    APH_PROFILER_SCOPE();
    if (m_declareData.passMap.contains(name))
    {
        RDG_LOG_ERR("The pass [%s] has been already created.", name);
        APH_ASSERT(false);
        return {};
    }
    auto* pass = m_resourcePool.renderPass.allocate(this, queueType, name);
    m_declareData.passMap[name] = pass;

    // Mark that passes have changed, affecting the graph topology
    markPassModified();

    if (isDryRunMode() && m_debugOutputEnabled)
    {
        RDG_LOG_INFO("[DryRun] Created pass '%s' with queue type %s", name, aph::vk::utils::toString(queueType));
    }

    return pass;
}

void RenderGraph::build(vk::SwapChain* pSwapChain)
{
    APH_PROFILER_SCOPE();

    if (!isDryRunMode())
    {
        if (pSwapChain != m_buildData.pSwapchain)
        {
            m_buildData.pSwapchain = pSwapChain;
            setDirty(DirtyFlagBits::SwapChainDirty);
        }
    }
    else if (m_debugOutputEnabled)
    {
        RDG_LOG_INFO("[DryRun] Building render graph (topology generation only)...");
    }

    // If nothing is dirty, no need to rebuild
    if (m_dirtyFlags == DirtyFlagBits::None)
    {
        return;
    }

    // Clear relevant data structures based on what's dirty
    if (isDirty(DirtyFlagBits::TopologyDirty | DirtyFlagBits::PassDirty))
    {
        if (!isDryRunMode())
        {
            std::lock_guard<std::mutex> holder{ m_buildData.submitLock };
            m_buildData.bufferBarriers.clear();
            m_buildData.imageBarriers.clear();
            m_buildData.frameSubmitInfos.clear();
        }
        m_buildData.sortedPasses.clear();
        m_buildData.currentResourceStates.clear();

        for (auto [name, pass] : m_declareData.passMap)
        {
            m_buildData.passDependencyGraph[pass].clear();
        }
    }

    if (isDirty(DirtyFlagBits::TopologyDirty | DirtyFlagBits::PassDirty))
    {
        APH_PROFILER_SCOPE_NAME("topological sort");

        if (isDryRunMode() && m_debugOutputEnabled)
        {
            RDG_LOG_INFO("[DryRun] Building dependency graph based on resources...");
        }

        for (auto [name, res] : m_declareData.resourceMap)
        {
            for (const auto& readPass : res->getReadPasses())
            {
                for (const auto& writePass : res->getWritePasses())
                {
                    if (readPass != writePass)
                    {
                        m_buildData.passDependencyGraph[readPass].insert(writePass);

                        if (isDryRunMode() && m_debugOutputEnabled)
                        {
                            RDG_LOG_DEBUG("[DryRun] %s depends on %s (resource: %s)", readPass->m_name,
                                          writePass->m_name, name);
                        }
                    }
                }
            }
        }

        if (m_buildData.passDependencyGraph.empty())
        {
            if (isDryRunMode() && m_debugOutputEnabled)
            {
                RDG_LOG_WARN("[DryRun] Warning: Render graph is empty (no dependencies found)");
            }
            else
            {
                RDG_LOG_WARN("render graph is empty.");
            }
        }

        // Perform topological sort
        if (isDryRunMode() && m_debugOutputEnabled)
        {
            RDG_LOG_INFO("[DryRun] Performing topological sort...");
        }

        HashMap<RenderPass*, int> inDegree;
        std::queue<RenderPass*> zeroInDegreeQueue;
        auto& sortedPasses = m_buildData.sortedPasses;
        auto& passDependencyGraph = m_buildData.passDependencyGraph;

        // Initialize in-degree of each node
        for (auto& [pass, nodes] : passDependencyGraph)
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

                if (isDryRunMode() && m_debugOutputEnabled)
                {
                    RDG_LOG_DEBUG("[DryRun] Starting node: %s (in-degree = 0)", pass->m_name);
                }
            }
        }

        // Topological Sort
        while (!zeroInDegreeQueue.empty())
        {
            RenderPass* node = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            sortedPasses.push_back(node);

            if (isDryRunMode() && m_debugOutputEnabled)
            {
                RDG_LOG_DEBUG("[DryRun] Adding to sorted order: %s", node->m_name);
            }

            // Decrease the in-degree of adjacent nodes
            for (RenderPass* adjacent : passDependencyGraph[node])
            {
                --inDegree[adjacent];
                if (inDegree[adjacent] == 0)
                {
                    zeroInDegreeQueue.push(adjacent);
                }
            }
        }

        // Check if there was a cycle in the graph
        bool hasCycle = sortedPasses.size() != passDependencyGraph.size();
        if (hasCycle)
        {
            if (isDryRunMode() && m_debugOutputEnabled)
            {
                RDG_LOG_ERR("[DryRun] ERROR: Cycle detected in the render graph!");
            }
            APH_ASSERT(!hasCycle);
        }
        else if (isDryRunMode() && m_debugOutputEnabled)
        {
            RDG_LOG_INFO("[DryRun] Topological sort completed successfully.");
        }
    }

    // Skip GPU resource allocation in dry run mode
    if (!isDryRunMode())
    {
        if (isDirty(DirtyFlagBits::ImageResourceDirty | DirtyFlagBits::BufferResourceDirty | DirtyFlagBits::PassDirty |
                    DirtyFlagBits::BackBufferDirty | DirtyFlagBits::SwapChainDirty))
        {
            // per pass resource build
            for (auto* pass : m_buildData.sortedPasses)
            {
                APH_PROFILER_SCOPE_NAME("pass resource build");

                // Acquire command buffers from the allocator if needed
                if (!m_buildData.cmds.contains(pass))
                {
                    m_buildData.cmds[pass] = m_pCommandBufferAllocator->acquire(pass->getQueueType());
                }

                // Create or update color attachments
                for (PassImageResource* colorAttachment : pass->m_res.colorOut)
                {
                    setupImageResource(colorAttachment, true);
                }

                // Create or update depth attachments
                if (pass->m_res.depthOut)
                {
                    setupImageResource(pass->m_res.depthOut, false);
                }

                // Initialize resource states for buffer resources
                {
                    auto resetResourceStates = [this](ArrayProxy<PassBufferResource*> resources)
                    {
                        APH_PROFILER_SCOPE();
                        for (auto* resource : resources)
                        {
                            if (!m_buildData.currentResourceStates.contains(resource))
                            {
                                m_buildData.currentResourceStates[resource] = ResourceState::Undefined;
                            }
                        }
                    };

                    resetResourceStates(pass->m_res.storageBufferIn);
                    resetResourceStates(pass->m_res.uniformBufferIn);
                    resetResourceStates(pass->m_res.storageBufferOut);
                }
            }
        }

        // Record commands for each pass
        if (isDirty(DirtyFlagBits::PassDirty | DirtyFlagBits::ImageResourceDirty | DirtyFlagBits::BufferResourceDirty |
                    DirtyFlagBits::TopologyDirty | DirtyFlagBits::SwapChainDirty))
        {
            vk::Fence* frameFence = m_buildData.frameExecuteFence;
            {
                frameFence->wait();
            }
            for (auto* pass : m_buildData.sortedPasses)
            {
                APH_PROFILER_SCOPE_NAME("pass commands recording");
                SmallVector<vk::ImageBarrier> initImageBarriers{};
                auto& imageBarriers = m_buildData.imageBarriers[pass];
                auto& bufferBarriers = m_buildData.bufferBarriers[pass];

                // Clear existing barriers
                imageBarriers.clear();
                bufferBarriers.clear();

                auto* pCmd = m_buildData.cmds[pass];
                APH_VR(pCmd->reset());
                APH_VR(pCmd->begin());

                // Collect attachment info
                vk::RenderingInfo renderingInfo{};
                {
                    auto& colorAttachmentInfos = renderingInfo.colors;
                    for (PassImageResource* colorAttachment : pass->m_res.colorOut)
                    {
                        auto pColorImage = m_buildData.image[colorAttachment];
                        vk::AttachmentInfo attachmentInfo = colorAttachment->getInfo().attachmentInfo;
                        attachmentInfo.image = pColorImage;
                        colorAttachmentInfos.push_back(attachmentInfo);
                        setupImageBarrier(initImageBarriers, colorAttachment, ResourceState::RenderTarget);
                    }

                    if (auto depthAttachment = pass->m_res.depthOut; depthAttachment)
                    {
                        vk::Image* pDepthImage = m_buildData.image[depthAttachment];
                        renderingInfo.depth = depthAttachment->getInfo().attachmentInfo;
                        renderingInfo.depth.image = pDepthImage;
                        setupImageBarrier(initImageBarriers, depthAttachment, ResourceState::DepthStencil);
                    }

                    pCmd->insertBarrier(initImageBarriers);
                }

                // setup resource barriers
                {
                    // Set up texture barriers
                    for (PassImageResource* textureIn : pass->m_res.textureIn)
                    {
                        ResourceState targetState = pass->m_res.resourceStateMap[textureIn];
                        setupResourceBarrier(imageBarriers, textureIn, targetState);
                    }

                    // Set up storage buffer barriers
                    for (PassBufferResource* bufferIn : pass->m_res.storageBufferIn)
                    {
                        ResourceState targetState = pass->m_res.resourceStateMap[bufferIn];
                        setupResourceBarrier(bufferBarriers, bufferIn, targetState);
                    }

                    // Set up uniform buffer barriers
                    for (PassBufferResource* bufferIn : pass->m_res.uniformBufferIn)
                    {
                        ResourceState targetState = pass->m_res.resourceStateMap[bufferIn];
                        setupResourceBarrier(bufferBarriers, bufferIn, targetState);
                    }
                }

                // Record remain commands
                {
                    APH_PROFILER_SCOPE_NAME("pass commands submit");
                    pCmd->insertDebugLabel({ .name = pass->m_name, .color = { 0.6f, 0.6f, 0.6f, 0.6f } });
                    pCmd->insertBarrier(m_buildData.bufferBarriers[pass], m_buildData.imageBarriers[pass]);

                    pCmd->beginRendering(renderingInfo);
                    if (!isDryRunMode())
                    {
                        APH_ASSERT(pass->m_executeCB);
                        pass->m_executeCB(pCmd);
                    }
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
    }
    else
    {
        // In dry run mode, just initialize resource states for tracking
        for (auto [name, res] : m_declareData.resourceMap)
        {
            if (!m_buildData.currentResourceStates.contains(res))
            {
                m_buildData.currentResourceStates[res] = ResourceState::Undefined;
            }
        }

        if (m_debugOutputEnabled)
        {
            RDG_LOG_INFO("[DryRun] Generated execution order:");
            for (uint32_t i = 1; auto* pass : m_buildData.sortedPasses)
            {
                RDG_LOG_INFO("[DryRun] %u. %s", i++, pass->m_name);
            }
        }
    }

    // All dirty flags have been handled
    clearDirtyFlags();
}

void RenderGraph::setupImageResource(PassImageResource* imageResource, bool isColorAttachment)
{
    APH_PROFILER_SCOPE();
    bool needsRebuild = !m_buildData.image.contains(imageResource);

    // Rebuild if image resource is dirty and this resource isn't external
    if (isDirty(DirtyFlagBits::ImageResourceDirty) && !(imageResource->getFlags() & PassResourceFlagBits::External))
    {
        needsRebuild = true;
    }

    if (needsRebuild)
    {
        // Clean up previous image if one exists
        if (m_buildData.image.contains(imageResource) && !(imageResource->getFlags() & PassResourceFlagBits::External))
        {
            m_pDevice->destroy(m_buildData.image[imageResource]);
        }

        vk::Image* pImage = {};
        vk::ImageCreateInfo createInfo{
            .extent = imageResource->getInfo().createInfo.extent,
            .usage = imageResource->getUsage(),
            // TODO
            .domain = MemoryDomain::Device,
            .imageType = ImageType::e2D,
            .format = imageResource->getInfo().createInfo.format,
        };

        // Add transfer source usage for color attachments that might be presented
        if (isColorAttachment && !m_declareData.backBuffer.empty() &&
            m_declareData.resourceMap.contains(m_declareData.backBuffer))
        {
            createInfo.usage |= ImageUsage::TransferSrc;
        }

        APH_VR(m_pDevice->create(createInfo, &pImage, imageResource->getName()));
        m_buildData.image[imageResource] = pImage;

        // Initialize resource state for newly created resources
        m_buildData.currentResourceStates[imageResource] = ResourceState::Undefined;
    }
}

void RenderGraph::setupImageBarrier(SmallVector<vk::ImageBarrier>& barriers, PassImageResource* resource,
                                    ResourceState newState)
{
    APH_PROFILER_SCOPE();
    auto& image = m_buildData.image[resource];
    ResourceState currentState = m_buildData.currentResourceStates[resource];

    barriers.push_back({
        .pImage = image,
        .currentState = currentState,
        .newState = newState,
    });

    // Update tracking
    m_buildData.currentResourceStates[resource] = newState;
}

template <typename BarrierType, typename ResourceType>
void RenderGraph::setupResourceBarrier(SmallVector<BarrierType>& barriers, ResourceType* resource,
                                       ResourceState targetState)
{
    APH_PROFILER_SCOPE();
    ResourceState currentState = m_buildData.currentResourceStates[resource];

    if (currentState != targetState)
    {
        if constexpr (std::is_same_v<ResourceType, PassImageResource>)
        {
            auto& image = m_buildData.image[resource];
            barriers.push_back(BarrierType{
                .pImage = image,
                .currentState = currentState,
                .newState = targetState,
            });
        }
        else if constexpr (std::is_same_v<ResourceType, PassBufferResource>)
        {
            auto& buffer = m_buildData.buffer[resource];
            barriers.push_back(BarrierType{
                .pBuffer = buffer,
                .currentState = currentState,
                .newState = targetState,
            });
        }

        // Update tracking
        m_buildData.currentResourceStates[resource] = targetState;
    }
}

PassResource* RenderGraph::importPassResource(const std::string& name, ResourcePtr resource)
{
    APH_PROFILER_SCOPE();

    if (isDryRunMode())
    {
        auto type =
            std::holds_alternative<vk::Buffer*>(resource) ? PassResource::Type::Buffer : PassResource::Type::Image;

        auto* res = createPassResource(name, type);

        if (m_debugOutputEnabled)
        {
            const char* typeStr = (type == PassResource::Type::Buffer) ? "Buffer" : "Image";
            RDG_LOG_INFO("[DryRun] Imported resource '%s' of type %s", name, typeStr);
        }

        res->addFlags(PassResourceFlagBits::External);
        m_buildData.currentResourceStates[res] = ResourceState::General;
        return res;
    }

    PassResource* passResource = std::visit(
        [this, &name](auto* ptr) -> PassResource*
        {
            using T = std::decay_t<decltype(*ptr)>;

            if constexpr (std::is_same_v<T, vk::Buffer>)
            {
                auto res = createPassResource(name, PassResource::Type::Buffer);
                APH_ASSERT(!m_buildData.buffer.contains(res));
                m_buildData.buffer[res] = ptr;
                markBufferResourcesModified();
                return res;
            }
            else if constexpr (std::is_same_v<T, vk::Image>)
            {
                auto res = createPassResource(name, PassResource::Type::Image);
                APH_ASSERT(!m_buildData.image.contains(res));
                m_buildData.image[res] = ptr;
                markImageResourcesModified();
                return res;
            }
            else
            {
                static_assert(dependent_false_v<T>, "Unsupported resource type");
                return nullptr;
            }
        },
        resource);

    passResource->addFlags(PassResourceFlagBits::External);
    m_buildData.currentResourceStates[passResource] = ResourceState::General;

    // Mark that the graph topology has changed
    markTopologyModified();

    return passResource;
}

PassResource* RenderGraph::createPassResource(const std::string& name, PassResource::Type type)
{
    APH_PROFILER_SCOPE();
    if (m_declareData.resourceMap.contains(name))
    {
        auto res = m_declareData.resourceMap.at(name);
        APH_ASSERT(res->getType() == type);
        return res;
    }

    PassResource* res = {};
    switch (type)
    {
    case PassResource::Type::Image:
        res = m_resourcePool.passImageResource.allocate(type);
        markImageResourcesModified();
        break;
    case PassResource::Type::Buffer:
        res = m_resourcePool.passBufferResource.allocate(type);
        markBufferResourcesModified();
        break;
    }
    res->setName(name);

    APH_ASSERT(res);

    m_declareData.resourceMap[name] = res;

    // Mark that the graph topology has changed
    markTopologyModified();

    if (isDryRunMode() && m_debugOutputEnabled)
    {
        const char* typeStr = (type == PassResource::Type::Buffer) ? "Buffer" : "Image";
        RDG_LOG_INFO("[DryRun] Created resource '%s' of type %s", name, typeStr);
    }

    return res;
}

void RenderGraph::execute(vk::Fence** ppFence)
{
    APH_PROFILER_SCOPE();

    if (isDryRunMode())
    {
        if (m_debugOutputEnabled)
        {
            RDG_LOG_INFO("[DryRun] Executing render graph simulation...");

            for (auto* pass : m_buildData.sortedPasses)
            {
                RDG_LOG_INFO("[DryRun] Executing pass: %s", pass->m_name);

                // Debug info about resources used by this pass
                if (!pass->m_res.textureIn.empty())
                {
                    RDG_LOG_DEBUG("[DryRun]   Reading textures: ");
                    for (auto* tex : pass->m_res.textureIn)
                    {
                        RDG_LOG_DEBUG("%s ", tex->getName());
                    }
                }

                if (!pass->m_res.colorOut.empty())
                {
                    RDG_LOG_DEBUG("[DryRun]   Writing color outputs: ");
                    for (auto* tex : pass->m_res.colorOut)
                    {
                        RDG_LOG_DEBUG("%s ", tex->getName());
                    }
                }

                if (pass->m_res.depthOut)
                {
                    RDG_LOG_DEBUG("[DryRun]   Writing depth output: %s", pass->m_res.depthOut->getName());
                }

                if (!pass->m_res.storageBufferIn.empty())
                {
                    RDG_LOG_DEBUG("[DryRun]   Reading storage buffers: ");
                    for (auto* buf : pass->m_res.storageBufferIn)
                    {
                        RDG_LOG_DEBUG("%s ", buf->getName());
                    }
                }

                if (!pass->m_res.uniformBufferIn.empty())
                {
                    RDG_LOG_DEBUG("[DryRun]   Reading uniform buffers: ");
                    for (auto* buf : pass->m_res.uniformBufferIn)
                    {
                        RDG_LOG_DEBUG("%s ", buf->getName());
                    }
                }

                if (!pass->m_res.storageBufferOut.empty())
                {
                    RDG_LOG_DEBUG("[DryRun]   Writing storage buffers: ");
                    for (auto* buf : pass->m_res.storageBufferOut)
                    {
                        RDG_LOG_DEBUG("%s ", buf->getName());
                    }
                }
            }

            RDG_LOG_INFO("[DryRun] Render graph execution completed");

            if (!m_declareData.backBuffer.empty())
            {
                RDG_LOG_INFO("[DryRun] Presenting back buffer: %s", m_declareData.backBuffer);
            }
        }

        return;
    }

    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    // submit && present
    {
        if (ppFence)
        {
            *ppFence = m_buildData.frameExecuteFence;
        }

        vk::Fence* frameFence = m_buildData.frameExecuteFence;
        {
            frameFence->wait();
            frameFence->reset();
        }

        APH_VR(queue->submit(m_buildData.frameSubmitInfos, frameFence));

        if (m_buildData.pSwapchain)
        {
            auto outImage = m_buildData.image[m_declareData.resourceMap[m_declareData.backBuffer]];

            // Update resource state for the back buffer to Present state
            m_buildData.currentResourceStates[m_declareData.resourceMap[m_declareData.backBuffer]] =
                ResourceState::Present;

            APH_VR(m_buildData.pSwapchain->presentImage({}, outImage));
        }
    }
}

void RenderGraph::setBackBuffer(const std::string& backBuffer)
{
    APH_PROFILER_SCOPE();
    m_declareData.backBuffer = backBuffer;

    // Mark that the back buffer has changed
    markBackBufferModified();

    if (isDryRunMode() && m_debugOutputEnabled)
    {
        RDG_LOG_INFO("[DryRun] Set back buffer to '%s'", backBuffer);
    }
}

void RenderGraph::cleanup()
{
    if (!isDryRunMode())
    {
        m_buildData.bufferBarriers.clear();
        m_buildData.imageBarriers.clear();
        m_buildData.frameSubmitInfos.clear();

        // Release the frame execute fence
        if (m_buildData.frameExecuteFence)
        {
            APH_VR(m_pDevice->releaseFence(m_buildData.frameExecuteFence));
            m_buildData.frameExecuteFence = nullptr;
        }

        // Clean up GPU resources
        for (auto [name, pResource] : m_declareData.resourceMap)
        {
            switch (pResource->getType())
            {
            case PassResource::Type::Image:
            {
                if (!(pResource->getFlags() & PassResourceFlagBits::External))
                {
                    auto pImage = m_buildData.image[pResource];
                    m_pDevice->destroy(pImage);
                }
            }
            break;
            case PassResource::Type::Buffer:
            {
                if (!(pResource->getFlags() & PassResourceFlagBits::External))
                {
                    auto pBuffer = m_buildData.buffer[pResource];
                    m_pDevice->destroy(pBuffer);
                }
            }
            break;
            }
        }

        // Release command buffers
        for (auto [pass, cmdBuffer] : m_buildData.cmds)
        {
            m_pCommandBufferAllocator->release(cmdBuffer);
        }
        m_buildData.cmds.clear();
    }

    // Clean up graph data structures in both modes
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
            m_resourcePool.passImageResource.free(static_cast<PassImageResource*>(pResource));
            break;
        case PassResource::Type::Buffer:
            m_resourcePool.passBufferResource.free(static_cast<PassBufferResource*>(pResource));
            break;
        }
    }
    m_declareData.resourceMap.clear();

    m_buildData.image.clear();
    m_buildData.buffer.clear();

    if (isDryRunMode() && m_debugOutputEnabled)
    {
        RDG_LOG_INFO("[DryRun] Cleaned up render graph");
    }
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
        node->addTableRow("Queue:", std::string{ aph::vk::utils::toString(pass->getQueueType()) });

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

PassResource* RenderGraph::getPassResource(const std::string& name) const
{
    APH_PROFILER_SCOPE();
    if (auto it = m_declareData.resourceMap.find(name); it != m_declareData.resourceMap.end())
    {
        return it->second;
    }
    return nullptr;
}
} // namespace aph
