#include "rendergraph.h"

namespace aph
{
RenderGraph::RenderGraph(vk::Device* pDevice) : m_pDevice(pDevice)
{
}

RenderPass* RenderGraph::addPass(std::string_view name, QueueType queueType)
{
    if(m_passToIndex.contains(name.data()))
    {
        return m_passes.at(m_passToIndex.at(name.data()));
    }

    std::size_t index = m_passes.size();
    m_passes.emplace_back(m_resourcePool.renderPass.allocate(this, index, queueType, name));
    m_passToIndex[name.data()] = index;
    return m_passes.back();
}

RenderPass* RenderGraph::getPass(std::string_view name)
{
    if(m_passToIndex.contains(name.data()))
    {
        return m_passes.at(m_passToIndex.at(name.data()));
    }
    return nullptr;
}

void RenderGraph::setOutput(uint32_t idx)
{
}

void RenderGraph::reset()
{
}

void RenderGraph::bake()
{
}
}  // namespace aph
