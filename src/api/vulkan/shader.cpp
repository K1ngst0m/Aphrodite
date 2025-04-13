#include "shader.h"

namespace aph::vk
{
Shader::Shader(const CreateInfoType& createInfo)
    : ResourceHandle({}, createInfo)
{
}

auto Shader::getEntryPointName() const -> std::string_view
{
    return getCreateInfo().entrypoint;
}

auto Shader::getStage() const -> ShaderStage
{
    return getCreateInfo().stage;
}

auto Shader::getCode() const -> const std::vector<uint32_t>&
{
    return getCreateInfo().code;
}

PipelineLayout::PipelineLayout(CreateInfoType createInfo, ::vk::PipelineLayout handle)
    : ResourceHandle(handle, createInfo)
{
}

auto PipelineLayout::getVertexInput() noexcept -> const VertexInput&
{
    return getCreateInfo().vertexInput;
}

auto PipelineLayout::getPushConstantRange() const noexcept -> const PushConstantRange&
{
    return getCreateInfo().pushConstantRange;
}

auto PipelineLayout::getSetLayouts() const noexcept -> const SmallVector<DescriptorSetLayout*>&
{
    return getCreateInfo().setLayouts;
}

auto PipelineLayout::getSetLayout(uint32_t setIdx) const noexcept -> DescriptorSetLayout*
{
    return getCreateInfo().setLayouts.at(setIdx);
}

ShaderProgram::ShaderProgram(CreateInfoType createInfo, HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps)
    : ResourceHandle({}, std::move(createInfo))
    , m_shaderObjects(std::move(shaderObjectMaps))
{
    m_pipelineType = utils::determinePipelineType(m_createInfo.shaders);
}

auto ShaderProgram::getVertexInput() const -> const VertexInput&
{
    return getPipelineLayout()->getVertexInput();
}

auto ShaderProgram::getSetLayout(uint32_t setIdx) const -> DescriptorSetLayout*
{
    if (getPipelineLayout()->getSetLayouts().size() > setIdx)
    {
        return getPipelineLayout()->getSetLayout(setIdx);
    }
    return nullptr;
}

auto ShaderProgram::getShader(ShaderStage stage) const -> Shader*
{
    const auto& shaders = getCreateInfo().shaders;
    if (shaders.contains(stage))
    {
        return shaders.at(stage);
    }
    return nullptr;
}

auto ShaderProgram::getShaderObject(ShaderStage stage) const -> ::vk::ShaderEXT
{
    if (m_shaderObjects.contains(stage))
    {
        return m_shaderObjects.at(stage);
    }
    return VK_NULL_HANDLE;
}

auto ShaderProgram::getPipelineLayout() const -> PipelineLayout*
{
    return m_createInfo.pPipelineLayout;
}

auto ShaderProgram::getPipelineType() const -> PipelineType
{
    return m_pipelineType;
}

auto ShaderProgram::getPushConstantRange() const -> const PushConstantRange&
{
    return getPipelineLayout()->getPushConstantRange();
}
} // namespace aph::vk

namespace aph::vk::utils
{
auto determinePipelineType(const HashMap<ShaderStage, Shader*>& shaders) -> PipelineType
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
