#pragma once

#include "api/vulkan/device.h"
#include "reflection/shaderReflector.h"

namespace aph
{
class ShaderAsset
{
public:
    // Construction/Destruction
    ShaderAsset();
    ~ShaderAsset();

    // Core resource access
    auto getProgram() const -> vk::ShaderProgram*;
    auto isValid() const -> bool;

    // Shader program information
    auto getPipelineType() const -> PipelineType;
    auto getPipelineLayout() const -> vk::PipelineLayout*;
    auto getShader(ShaderStage stage) const -> vk::Shader*;
    auto getSetLayout(uint32_t setIdx) const -> vk::DescriptorSetLayout*;
    auto getVertexInput() const -> const VertexInput&;
    auto getPushConstantRange() const -> const PushConstantRange&;

    // Reflection data
    auto getReflectionData() const -> const ReflectionResult&;
    void setReflectionData(const ReflectionResult& reflectionData);

    // Resource metadata
    auto getSourceDesc() const -> const std::string&;
    auto getDebugName() const -> const std::string&;
    auto getLoadTimestamp() const -> uint64_t;

    // Debug utilities
    auto getInfoString() const -> std::string;
    auto getPipelineTypeString() const -> std::string;

    // Internal resource management
    void setShaderProgram(vk::ShaderProgram* pProgram);
    void setLoadInfo(const std::string& sourceDesc, const std::string& debugName);

private:
    vk::ShaderProgram* m_pShaderProgram;
    ReflectionResult m_reflectionData;
    std::string m_sourceDesc; // Description of source (file paths)
    std::string m_debugName; // Debug name used for the resource
    uint64_t m_loadTimestamp; // When the shader was loaded
};
} // namespace aph
