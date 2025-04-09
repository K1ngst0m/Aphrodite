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
        return {Result::RuntimeError, "Device is required for RenderGraphComposer"};
    }

    // Create the instance
    auto* pComposer = new FrameComposer(createInfo);
    if (!pComposer)
    {
        return {Result::RuntimeError, "Failed to allocate RenderGraphComposer"};
    }

    // Initialize it
    Result result = pComposer->initialize(createInfo);
    if (!result.success())
    {
        Destroy(pComposer);
        return {result, "Failed to initialize RenderGraphComposer"};
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

    m_frameCount = frameCount;
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
    return {getCurrentGraph(), m_currentFrame};
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

    LoadRequest request = m_pResourceLoader->createRequest();
    for (auto graph : m_frameGraphs)
    {
        if (graph->m_declareData.pendingBufferLoad.empty() && graph->m_declareData.pendingImageLoad.empty())
        {
            continue;
        }

        for (auto& [name, pendingLoad] : graph->m_declareData.pendingImageLoad)
        {
            if (m_buildImage.contains(name))
            {
                RDG_LOG_WARN("Pending load of %s is already build, skip.", name);
                continue;
            }
            auto& imageAsset = m_buildImage[name];
            request.add(pendingLoad.loadInfo, &imageAsset);
            RDG_LOG_INFO("loading resource: %s", name);
            APH_ASSERT(m_buildImage.contains(name));
        }

        for (auto& [name, pendingLoad] : graph->m_declareData.pendingBufferLoad)
        {
            if (m_buildBuffer.contains(name))
            {
                RDG_LOG_WARN("Pending load of %s is already build, skip.", name);
                continue;
            }

            BufferAsset* bufferAsset = nullptr;
            m_buildBuffer[name] = bufferAsset;
            request.add(pendingLoad.loadInfo, &m_buildBuffer[name]);
            RDG_LOG_INFO("loading resource: %s", name);
            APH_ASSERT(m_buildBuffer.contains(name));
            // TODO find out
            request.load();
        }

        graph->m_declareData.pendingBufferLoad.clear();
        graph->m_declareData.pendingImageLoad.clear();
    }
    request.load();

    for (auto graph : m_frameGraphs)
    {
        for (auto& [name, pBufferAsset] : m_buildBuffer)
        {
            APH_ASSERT(pBufferAsset && pBufferAsset->isValid());
            graph->importPassResource(name, pBufferAsset->getBuffer());
        }

        for (auto& [name, pImageAsset] : m_buildImage)
        {
            APH_ASSERT(pImageAsset && pImageAsset->isValid());
            graph->importPassResource(name, pImageAsset->getImage());
        }
    }
}

void FrameComposer::buildAllGraphs(vk::SwapChain* pSwapChain)
{
    APH_PROFILER_SCOPE();

    // First, sync any shared resources
    syncSharedResources();

    // Then build all graphs
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
        setCurrentFrame(frameIndex);
        co_yield FrameResource{.pGraph = getCurrentGraph(), .frameIndex = frameIndex};
    }
    // TODO
    syncSharedResources();
}
uint32_t FrameComposer::getFrameCount() const
{
    return m_frameCount;
}
} // namespace aph
