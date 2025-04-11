#pragma once

#include "api/vulkan/device.h"

namespace aph
{
class ShaderAsset
{
public:
    ShaderAsset();
    ~ShaderAsset();

    // Access to the underlying resource
    vk::ShaderProgram* getProgram() const
    {
        return m_pShaderProgram;
    }

    // Accessors for shader information
    PipelineType getPipelineType() const;
    vk::PipelineLayout* getPipelineLayout() const;
    vk::Shader* getShader(ShaderStage stage) const;
    vk::DescriptorSetLayout* getSetLayout(uint32_t setIdx) const;
    const VertexInput& getVertexInput() const;
    const PushConstantRange& getPushConstantRange() const;

    // Mid-level loading info accessors
    const std::string& getSourceDesc() const
    {
        return m_sourceDesc;
    }
    const std::string& getDebugName() const
    {
        return m_debugName;
    }
    bool isValid() const
    {
        return m_pShaderProgram != nullptr;
    }
    uint64_t getLoadTimestamp() const
    {
        return m_loadTimestamp;
    }

    // Utility methods
    std::string getInfoString() const;
    std::string getPipelineTypeString() const;

    // Internal use by the shader loader
    void setShaderProgram(vk::ShaderProgram* pProgram);
    void setLoadInfo(const std::string& sourceDesc, const std::string& debugName);

private:
    vk::ShaderProgram* m_pShaderProgram;
    std::string m_sourceDesc; // Description of source (file paths)
    std::string m_debugName; // Debug name used for the resource
    uint64_t m_loadTimestamp; // When the shader was loaded
};
} // namespace aph
