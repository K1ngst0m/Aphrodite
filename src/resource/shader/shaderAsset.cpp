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
}

auto ShaderAsset::getProgram() const -> vk::ShaderProgram*
{
    return m_pShaderProgram;
}

auto ShaderAsset::getPipelineType() const -> PipelineType
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPipelineType();
    }
    return PipelineType::Undefined;
}

auto ShaderAsset::getPipelineLayout() const -> vk::PipelineLayout*
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPipelineLayout();
    }
    return nullptr;
}

auto ShaderAsset::getShader(ShaderStage stage) const -> vk::Shader*
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getShader(stage);
    }
    return nullptr;
}

auto ShaderAsset::getSetLayout(uint32_t setIdx) const -> vk::DescriptorSetLayout*
{
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getSetLayout(setIdx);
    }
    return nullptr;
}

auto ShaderAsset::getVertexInput() const -> const VertexInput&
{
    static VertexInput emptyInput;
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getVertexInput();
    }
    return emptyInput;
}

auto ShaderAsset::getPushConstantRange() const -> const PushConstantRange&
{
    static PushConstantRange emptyRange;
    if (m_pShaderProgram)
    {
        return m_pShaderProgram->getPushConstantRange();
    }
    return emptyRange;
}

auto ShaderAsset::getReflectionData() const -> const ReflectionResult&
{
    return m_reflectionData;
}

void ShaderAsset::setReflectionData(const ReflectionResult& reflectionData)
{
    m_reflectionData = reflectionData;
}

auto ShaderAsset::getSourceDesc() const -> const std::string&
{
    return m_sourceDesc;
}

auto ShaderAsset::getDebugName() const -> const std::string&
{
    return m_debugName;
}

auto ShaderAsset::isValid() const -> bool
{
    return m_pShaderProgram != nullptr;
}

auto ShaderAsset::getLoadTimestamp() const -> uint64_t
{
    return m_loadTimestamp;
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

auto ShaderAsset::getPipelineTypeString() const -> std::string
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

auto ShaderAsset::getInfoString() const -> std::string
{
    std::stringstream ss;

    // Basic shader properties
    ss << "Shader: " << (m_debugName.empty() ? "Unnamed" : m_debugName) << "\n";
    ss << "Type: " << getPipelineTypeString() << "\n";

    // List active shader stages
    ss << "Stages: ";
    bool hasStages = false;

    // Check for common shader stages
    const ShaderStage stages[] = { ShaderStage::VS, ShaderStage::FS, ShaderStage::CS, ShaderStage::MS,
                                   ShaderStage::TS };

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
