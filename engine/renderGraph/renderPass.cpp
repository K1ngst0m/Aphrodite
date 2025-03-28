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

PassBufferResource* RenderPass::addStorageBufferIn(const std::string& name, vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(BufferUsage::Storage);
    res->addAccessFlags(::vk::AccessFlagBits2::eShaderStorageRead);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferIn.push_back(res);

    if (pBuffer)
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
    VK_LOG_DEBUG("Pass '%s' added as READ pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(BufferUsage::Uniform);
    res->addAccessFlags(::vk::AccessFlagBits2::eShaderRead);

    m_res.resourceStateMap[res] = ResourceState::UniformBuffer;
    m_res.uniformBufferIn.push_back(res);

    if (pBuffer)
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
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for buffer '%s'", m_name.c_str(), name.c_str());
    res->addUsage(BufferUsage::Storage);
    res->addAccessFlags(::vk::AccessFlagBits2::eShaderWrite);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferOut.push_back(res);

    return res;
}
PassImageResource* RenderPass::addTextureOut(const std::string& name)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for texture '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::Storage);
    res->addAccessFlags(::vk::AccessFlagBits2::eShaderStorageWrite);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.textureOut.push_back(res);

    return res;
}

PassImageResource* RenderPass::addTextureIn(const std::string& name, vk::Image* pImage)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addReadPass(this);
    VK_LOG_DEBUG("Pass '%s' added as READ pass for texture '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::Sampled);
    res->addAccessFlags(::vk::AccessFlagBits2::eShaderSampledRead);

    m_res.resourceStateMap[res] = ResourceState::ShaderResource;
    m_res.textureIn.push_back(res);

    if (pImage)
    {
        m_pRenderGraph->importResource(name, pImage);
    }

    return res;
}

PassImageResource* RenderPass::setColorOut(const std::string& name, const vk::ImageCreateInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for color output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::ColorAttachment);
    m_res.resourceStateMap[res] = ResourceState::RenderTarget;
    m_res.colorOut.push_back(res);
    return res;
}

PassImageResource* RenderPass::setDepthStencilOut(const std::string& name, const vk::ImageCreateInfo& info)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    VK_LOG_DEBUG("Pass '%s' added as WRITE pass for depth output '%s'", m_name.c_str(), name.c_str());
    res->addUsage(ImageUsage::DepthStencil);
    m_res.resourceStateMap[res] = ResourceState::DepthStencil;
    m_res.depthOut = res;
    return res;
}

} // namespace aph
