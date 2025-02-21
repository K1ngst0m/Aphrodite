#include "shader.h"
#include "device.h"

namespace aph::vk
{

Shader::Shader(const CreateInfoType& createInfo, HandleType handle, ResourceLayout layout) :
    ResourceHandle(handle, createInfo),
    m_layout(layout)
{
}

ShaderProgram::ShaderProgram(const CreateInfoType& createInfo, const CombinedResourceLayout& layout,
                  VkPipelineLayout pipelineLayout, SmallVector<DescriptorSetLayout*> setLayouts) :
    ResourceHandle({}, createInfo),
    m_pSetLayouts(setLayouts),
    m_pipeLayout(pipelineLayout),
    m_combineLayout(layout)
{
    switch(getPipelineType())
    {
    case PipelineType::Geometry:
    {
        APH_ASSERT(createInfo.geometry.pVertex);
        APH_ASSERT(createInfo.geometry.pFragment);
        m_shaders[ShaderStage::VS] = createInfo.geometry.pVertex;
        m_shaders[ShaderStage::FS] = createInfo.geometry.pFragment;
    }
    break;
    case PipelineType::Mesh:
    {
        APH_ASSERT(createInfo.mesh.pMesh);
        APH_ASSERT(createInfo.mesh.pFragment);
        m_shaders[ShaderStage::MS] = createInfo.mesh.pMesh;
        if(createInfo.mesh.pTask)
        {
            m_shaders[ShaderStage::TS] = createInfo.mesh.pTask;
        }
        m_shaders[ShaderStage::FS] = createInfo.mesh.pFragment;
    }
    break;
    case PipelineType::Compute:
    {
        m_shaders[ShaderStage::CS] = createInfo.compute.pCompute;
    }
    break;
    default:
        APH_ASSERT(false);
        break;
    }

    createVertexInput();
}

VkShaderStageFlags ShaderProgram::getConstantShaderStage(uint32_t offset, uint32_t size) const
{
    VkShaderStageFlags stage = 0;
    size += offset;
    const auto& constant = m_combineLayout.pushConstantRange;
    stage |= constant.stageFlags;
    offset += constant.size;
    return stage;
}

void ShaderProgram::createVertexInput()
{
    if(getPipelineType() != PipelineType::Geometry && getPipelineType() != PipelineType::Mesh)
    {
        return;
    }
    uint32_t size = 0;
    aph::utils::forEachBit(m_combineLayout.attributeMask, [&](uint32_t location) {
        auto& attr = m_combineLayout.vertexAttr[location];
        m_vertexInput.attributes.push_back(
            {.location = location, .binding = 0, .format = utils::getFormatFromVk(attr.format), .offset = attr.offset});
        size += attr.size;
    });
    m_vertexInput.bindings.push_back({size});
}
}  // namespace aph::vk
