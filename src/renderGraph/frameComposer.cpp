#include "frameComposer.h"
#include "common/profiler.h"
#include "threads/taskManager.h"

namespace aph
{

FrameComposer::FrameComposer(const FrameComposerCreateInfo& createInfo)
    : m_pDevice(createInfo.pDevice)
    , m_pResourceLoader(createInfo.pResourceLoader)
    , m_frameCount(createInfo.frameCount)
{
}

FrameComposer::~FrameComposer()
{
    cleanup();
}

Expected<FrameComposer*> FrameComposer::Create(const FrameComposerCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Validate inputs
    if (!createInfo.pDevice)
    {
        return { Result::RuntimeError, "Device is required for RenderGraphComposer" };
    }

    // Create the instance
    auto* pComposer = new FrameComposer(createInfo);
    if (!pComposer)
    {
        return { Result::RuntimeError, "Failed to allocate RenderGraphComposer" };
    }

    // Initialize it
    Result result = pComposer->initialize(createInfo);
    if (!result.success())
    {
        Destroy(pComposer);
        return { result, "Failed to initialize RenderGraphComposer" };
    }

    return pComposer;
}

void FrameComposer::Destroy(FrameComposer* pComposer)
{
    if (pComposer)
    {
        pComposer->cleanup();
        delete pComposer;
    }
}

Result FrameComposer::initialize(const FrameComposerCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Set the frame count and create graphs
    m_frameCount = std::max(1u, createInfo.frameCount);

    // Create render graphs for each frame
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        Result result = createFrameGraph(i);
        if (!result.success())
        {
            return result;
        }
    }

    // Set current frame to 0
    m_currentFrame = 0;

    return Result::Success;
}

Result FrameComposer::createFrameGraph(uint32_t frameIndex)
{
    APH_PROFILER_SCOPE();

    // Create a new render graph
    auto result = RenderGraph::Create(m_pDevice);
    if (!result)
    {
        return result;
    }

    // Store it
    if (frameIndex >= m_frameGraphs.size())
    {
        m_frameGraphs.resize(frameIndex + 1, nullptr);
    }

    m_frameGraphs[frameIndex] = result.value();
    return Result::Success;
}

void FrameComposer::setFrameCount(uint32_t frameCount)
{
    APH_PROFILER_SCOPE();

    if (frameCount == 0)
    {
        RDG_LOG_ERR("Invalid frame count (0), defaulting to 1");
        frameCount = 1;
    }

    if (frameCount == m_frameCount)
    {
        return; // No change needed
    }

    // Clean up existing frames beyond the new count
    for (uint32_t i = frameCount; i < m_frameGraphs.size(); ++i)
    {
        if (m_frameGraphs[i])
        {
            RenderGraph::Destroy(m_frameGraphs[i]);
            m_frameGraphs[i] = nullptr;
        }
    }

    // Resize the vector
    m_frameGraphs.resize(frameCount, nullptr);

    // Create new frames if needed
    for (uint32_t i = 0; i < frameCount; ++i)
    {
        if (!m_frameGraphs[i])
        {
            APH_VERIFY_RESULT(createFrameGraph(i));
        }
    }

    m_frameCount   = frameCount;
    m_currentFrame = std::min(m_currentFrame, frameCount - 1);
}

void FrameComposer::setCurrentFrame(uint32_t frameIndex)
{
    if (frameIndex >= m_frameCount)
    {
        RDG_LOG_ERR("Invalid frame index %u (max: %u), defaulting to 0", frameIndex, m_frameCount - 1);
        frameIndex = 0;
    }

    m_currentFrame = frameIndex;
}

FrameComposer::FrameResource FrameComposer::getCurrentFrame() const
{
    return { getCurrentGraph(), m_currentFrame };
}

FrameComposer::FrameResource FrameComposer::nextFrame()
{
    APH_PROFILER_SCOPE();

    m_currentFrame = (m_currentFrame + 1) % m_frameCount;
    return getCurrentFrame();
}

RenderGraph* FrameComposer::getCurrentGraph() const
{
    return getGraph(m_currentFrame);
}

RenderGraph* FrameComposer::getGraph(uint32_t frameIndex) const
{
    APH_ASSERT(frameIndex < m_frameGraphs.size());
    if (frameIndex >= m_frameGraphs.size())
    {
        RDG_LOG_ERR("Invalid frame index %u (max: %u)", frameIndex, m_frameGraphs.size() - 1);
        return nullptr;
    }

    return m_frameGraphs[frameIndex];
}

void FrameComposer::syncSharedResources()
{
    APH_PROFILER_SCOPE();

    APH_ASSERT(!m_frameGraphs.empty());

    // Quick check if any graph has pending loads before doing any work
    bool hasPendingLoads   = false;
    bool hasPendingShaders = false;

    for (auto graph : m_frameGraphs)
    {
        if (!graph->m_declareData.pendingBufferLoad.empty() || !graph->m_declareData.pendingImageLoad.empty())
        {
            hasPendingLoads = true;
        }

        if (!graph->m_declareData.pendingShaderLoad.empty())
        {
            hasPendingShaders = true;
        }
    }

    // Early exit if nothing to load
    if (!hasPendingLoads && !hasPendingShaders)
    {
        return;
    }

    // Handle image and buffer resources
    if (hasPendingLoads)
    {
        LoadRequest request = m_pResourceLoader->createRequest();

        // First pass: Warm up the hashmaps with empty values
        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, _] : graph->m_declareData.pendingImageLoad)
            {
                if (!m_buildImage.contains(name))
                {
                    m_buildImage[name] = nullptr;
                }
            }

            for (auto& [name, _] : graph->m_declareData.pendingBufferLoad)
            {
                if (!m_buildBuffer.contains(name))
                {
                    m_buildBuffer[name] = nullptr;
                }
            }
        }

        {
            std::size_t size = m_buildImage.size();
            m_buildImage.clear();
            m_buildImage.reserve(size);
        }

        {
            std::size_t size = m_buildBuffer.size();
            m_buildBuffer.clear();
            m_buildBuffer.reserve(size);
        }

        // Second pass: Add resources to load request
        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, pendingLoad] : graph->m_declareData.pendingImageLoad)
            {
                // Skip if already loaded or being loaded in this batch
                if (m_buildImage.contains(name))
                {
                    RDG_LOG_DEBUG("Pending load of %s has already been loaded or queued, skip.", name);
                    continue;
                }

                if (pendingLoad.loadInfo.debugName.empty())
                {
                    pendingLoad.loadInfo.debugName = name;
                }

                if (pendingLoad.preCallback)
                {
                    pendingLoad.preCallback();
                }

                // Now safe to take address since hashmap won't rehash
                request.add(pendingLoad.loadInfo, &m_buildImage[name]);
                RDG_LOG_INFO("loading image resource: %s", name);
            }

            for (auto& [name, pendingLoad] : graph->m_declareData.pendingBufferLoad)
            {
                // Skip if already loaded or being loaded in this batch
                if (m_buildBuffer.contains(name))
                {
                    RDG_LOG_DEBUG("Pending load of %s has already been loaded or queued, skip.", name);
                    continue;
                }

                if (pendingLoad.loadInfo.debugName.empty())
                {
                    pendingLoad.loadInfo.debugName = name;
                }

                if (pendingLoad.preCallback)
                {
                    pendingLoad.preCallback();
                }

                // Now safe to take address since hashmap won't rehash
                request.add(pendingLoad.loadInfo, &m_buildBuffer[name]);
                RDG_LOG_INFO("loading buffer resource: %s", name);
            }
        }

        // Load all resources in one batch
        request.load();

        // Import resources into all graphs
        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, pBufferAsset] : m_buildBuffer)
            {
                APH_ASSERT(pBufferAsset != nullptr && pBufferAsset->isValid());
                graph->importPassResource(name, pBufferAsset->getBuffer());

                if (graph->m_declareData.pendingBufferLoad.contains(name) &&
                    graph->m_declareData.pendingBufferLoad[name].postCallback)
                {
                    graph->m_declareData.pendingBufferLoad[name].postCallback();
                }
            }

            for (auto& [name, pImageAsset] : m_buildImage)
            {
                APH_ASSERT(pImageAsset != nullptr && pImageAsset->isValid());
                graph->importPassResource(name, pImageAsset->getImage());

                if (graph->m_declareData.pendingImageLoad.contains(name) &&
                    graph->m_declareData.pendingImageLoad[name].postCallback)
                {
                    graph->m_declareData.pendingImageLoad[name].postCallback();
                }
            }
        }

        // Clean up
        for (auto graph : m_frameGraphs)
        {
            graph->m_declareData.pendingBufferLoad.clear();
            graph->m_declareData.pendingImageLoad.clear();
        }
    }

    // Handle shader loading
    if (hasPendingShaders)
    {
        LoadRequest shaderRequest = m_pResourceLoader->createRequest();

        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, _] : graph->m_declareData.pendingShaderLoad)
            {
                if (!m_buildShader.contains(name))
                {
                    m_buildShader[name] = nullptr;
                }
            }
        }

        // reserve to avoid rehashing
        {
            std::size_t size = m_buildShader.size();
            m_buildShader.clear();
            m_buildShader.reserve(size);
        }

        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, pendingLoad] : graph->m_declareData.pendingShaderLoad)
            {
                // Skip if already loaded or being loaded in this batch
                if (m_buildShader.contains(name))
                {
                    RDG_LOG_DEBUG("Pending load of %s has already been loaded or queued, skip.", name);
                    continue;
                }

                if (pendingLoad.loadInfo.debugName.empty())
                {
                    pendingLoad.loadInfo.debugName = name;
                }

                // Execute pre-callback if provided
                if (pendingLoad.preCallback)
                {
                    pendingLoad.preCallback();
                }

                shaderRequest.add(pendingLoad.loadInfo, &m_buildShader[name]);

                RDG_LOG_INFO("Adding shader to load request from graph: %s", name);
            }
        }

        // Load all shaders in one batch
        shaderRequest.load();

        for (auto graph : m_frameGraphs)
        {
            for (auto& [name, shaderAsset] : m_buildShader)
            {
                APH_ASSERT(shaderAsset != nullptr && shaderAsset->isValid());
                graph->importShader(name, shaderAsset->getProgram());

                // Execute post-callback if available for this resource
                if (graph->m_declareData.pendingShaderLoad.contains(name) &&
                    graph->m_declareData.pendingShaderLoad[name].postCallback)
                {
                    graph->m_declareData.pendingShaderLoad[name].postCallback();
                }
            }

            // Clear the pending shader loads from this graph
            graph->m_declareData.pendingShaderLoad.clear();
        }
    }
}

void FrameComposer::buildAllGraphs(vk::SwapChain* pSwapChain)
{
    APH_PROFILER_SCOPE();

    syncSharedResources();

    for (auto* graph : m_frameGraphs)
    {
        if (graph)
        {
            graph->build(pSwapChain);
        }
    }
}

void FrameComposer::cleanup()
{
    APH_PROFILER_SCOPE();

    // Destroy all render graphs
    for (auto* graph : m_frameGraphs)
    {
        if (graph)
        {
            RenderGraph::Destroy(graph);
        }
    }

    m_frameGraphs.clear();
}

coro::generator<FrameComposer::FrameResource> FrameComposer::frames()
{
    for (uint32_t frameIndex = 0; frameIndex < m_frameCount; ++frameIndex)
    {
        co_yield FrameResource{ .pGraph = getGraph(frameIndex), .frameIndex = frameIndex };
    }
    syncSharedResources();
}

uint32_t FrameComposer::getFrameCount() const
{
    return m_frameCount;
}
} // namespace aph
