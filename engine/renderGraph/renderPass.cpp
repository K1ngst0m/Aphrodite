#include "renderPass.h"
#include "renderGraph.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name)
    : m_pRenderGraph(pRDG)
    , m_index(index)
    , m_queueType(queueType)
    , m_name(name)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pRDG);
}

PassBufferResource* RenderPass::addStorageBufferIn(const std::string& name, vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    res->addUsage(BufferUsage::Storage);
    res->addAccessFlags(VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

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
    res->addUsage(BufferUsage::Uniform);
    res->addAccessFlags(VK_ACCESS_2_SHADER_READ_BIT);

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
    res->addUsage(BufferUsage::Storage);
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
    res->addUsage(ImageUsage::Storage);
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
    res->addUsage(ImageUsage::Sampled);
    res->addAccessFlags(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::ShaderResource;
    m_res.textureIn.push_back(res);

    if (pImage)
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
    res->addUsage(ImageUsage::ColorAttachment);
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
    res->addUsage(ImageUsage::DepthStencil);
    m_res.resourceStateMap[res] = ResourceState::DepthStencil;
    m_res.depthOut = res;
    return res;
}

}
