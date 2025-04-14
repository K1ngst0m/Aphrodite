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

PassBufferResource* RenderPass::addBufferIn(const BufferResourceInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res =
        static_cast<PassBufferResource*>(m_pRenderGraph->createPassResource(info.name, PassResource::Type::eBuffer));

    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for buffer '%s'", m_name.c_str(), info.name.c_str());
    res->addUsage(info.usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(info.usage, false);

    // Track special collection membership based on usage
    if (info.usage & BufferUsage::Uniform)
    {
        m_resource.uniformBufferIn.push_back(res);
    }
    else if (info.usage & BufferUsage::Storage)
    {
        m_resource.storageBufferIn.push_back(res);
    }

    res->addAccessFlags(accessFlags);
    m_resource.resourceStateMap[res] = state;

    // Handle resource importing or deferred loading
    if (std::holds_alternative<vk::Buffer*>(info.resource))
    {
        auto pBuffer = std::get<vk::Buffer*>(info.resource);
        if (pBuffer)
        {
            m_pRenderGraph->importPassResource(info.name, pBuffer);
        }
    }
    else if (std::holds_alternative<BufferLoadInfo>(info.resource))
    {
        const auto& loadInfo = std::get<BufferLoadInfo>(info.resource);
        // Add to pending loads list
        APH_ASSERT(!m_pRenderGraph->m_declareData.pendingBufferLoad.contains(info.name));
        m_pRenderGraph->m_declareData.pendingBufferLoad[info.name] = { info.name, loadInfo,         info.usage,
                                                                       res,       info.preCallback, info.postCallback };
    }

    if (info.shared)
    {
        markResourceAsShared(info.name);
    }

    // Mark the graph as having buffer resources changed
    m_pRenderGraph->markResourcesChanged(PassResource::Type::eBuffer);

    return res;
}

PassBufferResource* RenderPass::addBufferOut(const std::string& name, BufferUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::eBuffer));
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, true);

    // Track special collection membership based on usage
    if (usage & BufferUsage::Storage)
    {
        m_resource.storageBufferOut.push_back(res);
    }

    res->addAccessFlags(accessFlags);
    m_resource.resourceStateMap[res] = state;

    m_pRenderGraph->markResourcesChanged(PassResource::Type::eBuffer);

    return res;
}

PassImageResource* RenderPass::addTextureIn(const ImageResourceInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res =
        static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(info.name, PassResource::Type::eImage));

    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for texture '%s'", m_name.c_str(), info.name.c_str());
    res->addUsage(info.usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(info.usage, false);

    res->addAccessFlags(accessFlags);
    m_resource.resourceStateMap[res] = state;
    m_resource.textureIn.push_back(res);

    // Handle resource importing or deferred loading
    if (std::holds_alternative<vk::Image*>(info.resource))
    {
        auto pImage = std::get<vk::Image*>(info.resource);
        if (pImage)
        {
            m_pRenderGraph->importPassResource(info.name, pImage);
        }
    }
    else if (std::holds_alternative<ImageLoadInfo>(info.resource))
    {
        const auto& loadInfo = std::get<ImageLoadInfo>(info.resource);
        // Add to pending loads list
        APH_ASSERT(!m_pRenderGraph->m_declareData.pendingImageLoad.contains(info.name));
        m_pRenderGraph->m_declareData.pendingImageLoad[info.name] = { info.name, loadInfo,         info.usage,
                                                                      res,       info.preCallback, info.postCallback };
    }

    if (info.shared)
    {
        markResourceAsShared(info.name);
    }

    // Mark the graph as having image resources changed
    m_pRenderGraph->markResourcesChanged(PassResource::Type::eImage);

    return res;
}

PassImageResource* RenderPass::addTextureOut(const std::string& name, ImageUsage usage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::eImage));
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for texture '%s'", m_name.c_str(), name.c_str());
    res->addUsage(usage);

    // Get appropriate access flags and resource state based on usage
    auto [state, accessFlags] = vk::utils::getResourceState(usage, true);

    res->addAccessFlags(accessFlags);
    m_resource.resourceStateMap[res] = state;
    m_resource.textureOut.push_back(res);

    m_pRenderGraph->markResourcesChanged(PassResource::Type::eImage);

    return res;
}

void RenderPass::addShader(const ShaderResourceInfo& info)
{
    APH_PROFILER_SCOPE();

    if (std::holds_alternative<vk::ShaderProgram*>(info.resource))
    {
        // Handle direct shader program
        auto pShader = std::get<vk::ShaderProgram*>(info.resource);
        APH_ASSERT(false, "Direct shader program not supported yet.");
    }
    else if (std::holds_alternative<ShaderLoadInfo>(info.resource))
    {
        const auto& loadInfo = std::get<ShaderLoadInfo>(info.resource);
        // Add to pending shader loads
        m_pRenderGraph->m_declareData.pendingShaderLoad[info.name] = { info.name, loadInfo, info.preCallback,
                                                                       info.postCallback };
    }
}

PassImageResource* RenderPass::setColorOut(const std::string& name, const RenderPassAttachmentInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::eImage));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for color output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::ColorAttachment);
    m_resource.resourceStateMap[res] = ResourceState::RenderTarget;
    m_resource.colorOut.push_back(res);

    m_pRenderGraph->markResourcesChanged(PassResource::Type::eImage);

    return res;
}

PassImageResource* RenderPass::setDepthStencilOut(const std::string& name, const RenderPassAttachmentInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->createPassResource(name, PassResource::Type::eImage));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for depth output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::DepthStencil);
    m_resource.resourceStateMap[res] = ResourceState::DepthStencil;
    m_resource.depthOut              = res;

    m_pRenderGraph->markResourcesChanged(PassResource::Type::eImage);

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
    m_executionMode     = ExecutionMode::eConditional;
    m_conditionCallback = std::move(condition);
    m_pRenderGraph->markPassModified();
}

void RenderPass::setCulled(bool culled)
{
    m_executionMode = culled ? ExecutionMode::eCulled : ExecutionMode::eAlways;
    m_pRenderGraph->markPassModified();
}

bool RenderPass::shouldExecute() const
{
    if (m_executionMode == ExecutionMode::eAlways)
    {
        return true;
    }
    if (m_executionMode == ExecutionMode::eCulled)
    {
        return false;
    }
    return m_conditionCallback ? m_conditionCallback() : true;
}

void RenderPass::markResourceAsShared(const std::string& resourceName)
{
    auto* resource = m_pRenderGraph->getPassResource(resourceName);
    APH_ASSERT(resource);
    resource->addFlags(PassResourceFlagBits::eShared);
}

QueueType RenderPass::getQueueType() const
{
    return m_queueType;
}

void RenderPass::resetCommand()
{
    m_executeCB = {};
    m_recordList.clear();
    m_pRenderGraph->markPassModified();
}

void RenderPass::recordCommand(const std::string& shaderName, ExecuteCallBack&& callback)
{
    m_recordList.push_back({ .shaderName = shaderName, .callback = std::move(callback) });
    m_pRenderGraph->markPassModified();
}

auto RenderPass::Builder::build() -> RenderPass*
{
    return m_pass;
}

auto RenderPass::Builder::execute(ExecuteCallBack&& callback) -> Builder&
{
    m_pass->recordExecute(std::move(callback));
    return *this;
}

auto RenderPass::Builder::attachment(const std::string& name, const RenderPassAttachmentInfo& info, bool isDepth)
    -> Builder&
{
    if (isDepth)
        m_pass->setDepthStencilOut(name, info);
    else
        m_pass->setColorOut(name, info);
    return *this;
}

auto RenderPass::configure() -> Builder
{
    return Builder{ this };
}
} // namespace aph
