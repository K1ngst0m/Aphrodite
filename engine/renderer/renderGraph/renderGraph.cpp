#include "renderGraph.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name) :
    m_pRenderGraph(pRDG),
    m_index(index),
    m_queueType(queueType),
    m_name(name)
{
}
RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    if(m_renderPassMap.contains(name))
    {
        return m_passes[m_renderPassMap[name]];
    }

    auto  index = m_passes.size();
    auto* node  = m_resourcePool.renderPass.allocate(this, index, queueType, name);
    m_passes.emplace_back(node);
    m_renderPassMap[name.data()] = index;
    return node;
}

RenderGraph::RenderGraph(vk::Device* pDevice) : m_pDevice(pDevice)
{
}
}  // namespace aph
