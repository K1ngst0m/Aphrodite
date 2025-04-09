#include "renderPass.h"
#include "renderGraph.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pGraph, QueueType queueType, std::string_view name)
    : m_pRenderGraph(pGraph)
    , m_queueType(queueType)
    , m_name(name)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pGraph);
}

PassBufferResource* RenderPass::addBufferIn(const std::string& name, vk::Buffer* pBuffer, BufferUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, false);

    // Track special collection membership based on usage
    if (usage & BufferUsage::Uniform)
    {
        m_res.uniformBufferIn.push_back(res);
    }
    else if (usage & BufferUsage::Storage)
    {
        m_res.storageBufferIn.push_back(res);
    }

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;

    if (pBuffer)
    {
        m_pRenderGraph->importPassResource(name, pBuffer);
    }

    // Mark the graph as having buffer resources changed
    m_pRenderGraph->markResourcesChanged(PassResource::Type::Buffer);

    return res;
}

PassBufferResource* RenderPass::addBufferIn(const std::string& name, const BufferLoadInfo& loadInfo, BufferUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for buffer '%s' (deferred loading)", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, false);

    // Track special collection membership based on usage
    if (usage & BufferUsage::Uniform)
    {
        m_res.uniformBufferIn.push_back(res);
    }
    else if (usage & BufferUsage::Storage)
    {
        m_res.storageBufferIn.push_back(res);
    }

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;

    // Add to pending loads list
    m_res.pendingBufferLoads.push_back({name, loadInfo, usage, res});

    // Mark the graph as having buffer resources changed
    m_pRenderGraph->markResourcesChanged(PassResource::Type::Buffer);

    return res;
}

PassBufferResource* RenderPass::addBufferOut(const std::string& name, BufferUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Buffer));
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, true);

    // Track special collection membership based on usage
    if (usage & BufferUsage::Storage)
    {
        m_res.storageBufferOut.push_back(res);
    }

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Buffer);

    return res;
}

PassImageResource* RenderPass::addTextureOut(const std::string& name, ImageUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Image));
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for texture '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, true);

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;
    m_res.textureOut.push_back(res);

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Image);

    return res;
}

PassImageResource* RenderPass::addTextureIn(const std::string& name, vk::Image* pImage, ImageUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Image));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for texture '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, false);

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;
    m_res.textureIn.push_back(res);

    if (pImage)
    {
        m_pRenderGraph->importPassResource(name, pImage);
    }

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Image);

    return res;
}

PassImageResource* RenderPass::addTextureIn(const std::string& name, const ImageLoadInfo& loadInfo, ImageUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Image));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for texture '%s' (deferred loading)", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, false);

    res->addAccessFlags(accessFlags);
    m_res.resourceStateMap[res] = state;
    m_res.textureIn.push_back(res);

    // Add to pending loads list
    m_res.pendingImageLoads.push_back({name, loadInfo, usage, res});

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Image);

    return res;
}

PassImageResource* RenderPass::setColorOut(const std::string& name, const RenderPassAttachmentInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for color output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::ColorAttachment);
    m_res.resourceStateMap[res] = ResourceState::RenderTarget;
    m_res.colorOut.push_back(res);

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Image);

    return res;
}

PassImageResource* RenderPass::setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for depth output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::DepthStencil);
    m_res.resourceStateMap[res] = ResourceState::DepthStencil;
    m_res.depthOut = res;

    m_pRenderGraph->markResourcesChanged(PassResource::Type::Image);

    return res;
}

void RenderPass::recordExecute(ExecuteCallBack&& cb)
{
    m_executeCB = std::move(cb);
    m_pRenderGraph->markPassModified();
}
void RenderPass::recordClear(ClearColorCallBack&& cb)
{
    m_clearColorCB = std::move(cb);
    m_pRenderGraph->markPassModified();
}
void RenderPass::recordDepthStencil(ClearDepthStencilCallBack&& cb)
{
    m_clearDepthStencilCB = std::move(cb);
    m_pRenderGraph->markPassModified();
}
void RenderPass::setExecutionCondition(std::function<bool()>&& condition)
{
    m_executionMode = ExecutionMode::Conditional;
    m_conditionCallback = std::move(condition);
    m_pRenderGraph->markPassModified();
}
void RenderPass::setCulled(bool culled)
{
    m_executionMode = culled ? ExecutionMode::Culled : ExecutionMode::Always;
    m_pRenderGraph->markPassModified();
}
bool RenderPass::shouldExecute() const
{
    if (m_executionMode == ExecutionMode::Always)
        return true;
    if (m_executionMode == ExecutionMode::Culled)
        return false;
    return m_conditionCallback ? m_conditionCallback() : true;
}

void RenderPass::processResourceLoads()
{
    APH_PROFILER_SCOPE();

    auto* pResourceLoader = m_pRenderGraph->getResourceLoader();

    // Skip if no resource loader is available
    if (!pResourceLoader)
    {
        if (!m_res.pendingBufferLoads.empty() || !m_res.pendingImageLoads.empty())
        {
            VK_LOG_ERR("Pass '%s' has pending resource loads but no ResourceLoader is available", m_name);
            APH_ASSERT(false);
        }
        return;
    }

    // Create a load request for all pending resources
    if (!m_res.pendingBufferLoads.empty() || !m_res.pendingImageLoads.empty())
    {
        LoadRequest request = pResourceLoader->createRequest();

        // Temporary storage for loaded assets
        std::vector<std::pair<std::string, BufferAsset*>> loadedBuffers;
        std::vector<std::pair<std::string, ImageAsset*>> loadedImages;

        // Process buffer loads
        for (auto& pendingLoad : m_res.pendingBufferLoads)
        {
            loadedBuffers.push_back({pendingLoad.name, {}});
            request.add(pendingLoad.loadInfo, &loadedBuffers.back().second);
        }

        // Process image loads
        for (auto& pendingLoad : m_res.pendingImageLoads)
        {
            loadedImages.push_back({pendingLoad.name, {}});
            request.add(pendingLoad.loadInfo, &loadedImages.back().second);
        }

        // Execute all loads
        request.load();

        // Import loaded resources into render graph
        for (auto& [name, pBufferAsset] : loadedBuffers)
        {
            if (pBufferAsset && pBufferAsset->isValid())
            {
                m_pRenderGraph->importPassResource(name, pBufferAsset->getBuffer());
            }
        }

        for (auto& [name, pImageAsset] : loadedImages)
        {
            if (pImageAsset && pImageAsset->isValid())
            {
                m_pRenderGraph->importPassResource(name, pImageAsset->getImage());
            }
        }

        // Clear pending loads
        m_res.pendingBufferLoads.clear();
        m_res.pendingImageLoads.clear();
    }
}
} // namespace aph
