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
    m_pipelineType = utils::determinePipelineType(m_createInfo.shaders);
}
} // namespace aph::vk

namespace aph::vk::utils
{
PipelineType determinePipelineType(const HashMap<ShaderStage, Shader*>& shaders)
{
    // vs + fs
    if (shaders.contains(ShaderStage::VS) && shaders.contains(ShaderStage::FS))
    {
        return PipelineType::Geometry;
    }
    // ms + fs
    else if (shaders.contains(ShaderStage::MS) && shaders.contains(ShaderStage::FS))
    {
        return PipelineType::Mesh;
    }
    // cs
    else if (shaders.contains(ShaderStage::CS))
    {
        return PipelineType::Compute;
    }
    else
    {
        APH_ASSERT(false);
        return PipelineType::Undefined;
    }
}
} // namespace aph::vk::utils
