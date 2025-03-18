#include "shader.h"

namespace aph::vk
{
Shader::Shader(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
{
}

ShaderProgram::ShaderProgram(CreateInfoType createInfo, const PipelineLayout& layout,
                             HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps)
    : ResourceHandle({}, std::move(createInfo))
    , m_shaderObjects(std::move(shaderObjectMaps))
    , m_pipelineLayout(std::move(layout))
{
}
} // namespace aph::vk
