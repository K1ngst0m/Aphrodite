#include "shaderAsset.h"
#include "common/profiler.h"
#include "shaderLoader.h"

namespace aph
{
ShaderAsset::ShaderAsset()
    : m_pShaderProgram(nullptr)
    , m_loadTimestamp(0)
{
}

ShaderAsset::~ShaderAsset()
{
    // The ResourceLoader is responsible for freeing the shader program
}

PipelineType ShaderAsset::getPipelineType() const
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPipelineType();
    }
    return PipelineType::Undefined;
}

vk::PipelineLayout* ShaderAsset::getPipelineLayout() const
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPipelineLayout();
    }
    return nullptr;
}

vk::Shader* ShaderAsset::getShader(ShaderStage stage) const
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getShader(stage);
    }
    return nullptr;
}

vk::DescriptorSetLayout* ShaderAsset::getSetLayout(uint32_t setIdx) const
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getSetLayout(setIdx);
    }
    return nullptr;
}

const VertexInput& ShaderAsset::getVertexInput() const
{
    static VertexInput emptyInput;
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getVertexInput();
    }
    return emptyInput;
}

const PushConstantRange& ShaderAsset::getPushConstantRange() const
{
    static PushConstantRange emptyRange;
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPushConstantRange();
    }
    return emptyRange;
}

void ShaderAsset::setShaderProgram(vk::ShaderProgram* pProgram)
{
    m_pShaderProgram = pProgram;
}

void ShaderAsset::setLoadInfo(const std::string& sourceDesc, const std::string& debugName)
{
    m_sourceDesc    = sourceDesc;
    m_debugName     = debugName;
    m_loadTimestamp = std::chrono::steady_clock::now().time_since_epoch().count();
}

std::string ShaderAsset::getPipelineTypeString() const
{
    switch (getPipelineType())
    {
    case PipelineType::Geometry:
        return "Graphics (Vertex+Fragment)";
    case PipelineType::Mesh:
        return "Graphics (Mesh+Fragment)";
    case PipelineType::Compute:
        return "Compute";
    default:
        return "Undefined";
    }
}

std::string ShaderAsset::getInfoString() const
{
    std::stringstream ss;

    // Basic shader properties
    ss << "Shader: " << (m_debugName.empty() ? "Unnamed" : m_debugName) << "\n";
    ss << "Type: " << getPipelineTypeString() << "\n";

    // List active shader stages
    ss << "Stages: ";
    bool hasStages = false;

    // Check for common shader stages
    const ShaderStage stages[] = {ShaderStage::VS, ShaderStage::FS, ShaderStage::CS, ShaderStage::MS, ShaderStage::TS};

    for (auto stage : stages)
    {
        if (getShader(stage))
        {
            if (hasStages)
                ss << ", ";
            ss << vk::utils::toString(stage);
            hasStages = true;
        }
    }

    if (!hasStages)
        ss << "None";
    ss << "\n";

    // Source info
    ss << "Source: " << (m_sourceDesc.empty() ? "Unknown" : m_sourceDesc);

    return ss.str();
}

} // namespace aph
