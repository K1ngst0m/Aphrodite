#include "shader.h"

namespace aph::vk
{
Shader::Shader(const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo)
{}

ShaderProgram::ShaderProgram(const CreateInfoType& createInfo, const PipelineLayout& layout,
                  HashMap<ShaderStage, VkShaderEXT> shaderObjectMaps) :
    ResourceHandle({}, createInfo),
    m_shaderObjects(std::move(shaderObjectMaps)),
    m_pipelineLayout(std::move(layout))
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
}
}  // namespace aph::vk
