#include "shader.h"

namespace aph::vk
{
Shader::Shader(const CreateInfoType& createInfo)
    : ResourceHandle({}, createInfo)
{
}

ShaderProgram::ShaderProgram(CreateInfoType createInfo, HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps)
    : ResourceHandle({}, std::move(createInfo))
    , m_shaderObjects(std::move(shaderObjectMaps))
{
    // vs + fs
    if (m_createInfo.shaders.contains(ShaderStage::VS) && m_createInfo.shaders.contains(ShaderStage::FS))
    {
        m_pipelineType = PipelineType::Geometry;
    }
    else if (m_createInfo.shaders.contains(ShaderStage::MS) && m_createInfo.shaders.contains(ShaderStage::FS))
    {
        m_pipelineType = PipelineType::Mesh;
    }
    // cs
    else if (m_createInfo.shaders.contains(ShaderStage::CS))
    {
        m_pipelineType = PipelineType::Compute;
    }
    else
    {
        APH_ASSERT(false);
    }
}
} // namespace aph::vk
