#pragma once

#include "allocator/objectPool.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include "vkUtils.h"
#include <bitset>

namespace aph::vk
{
class Sampler;
class ImmutableSampler;
class DescriptorSetLayout;
class DescriptorSet;

struct PipelineLayout
{
    VertexInput vertexInput = {};
    ::vk::PushConstantRange pushConstantRange = {};

    SmallVector<DescriptorSetLayout*> setLayouts = {};
    ::vk::PipelineLayout handle = {};
    PipelineType type = {};
};

struct ImmutableSamplerBank
{
    Sampler* samplers[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
};

struct ShaderCreateInfo
{
    std::vector<uint32_t> code;
    std::string entrypoint = "main";
    aph::ShaderStage stage;
    bool compile = false;
};

class Shader : public ResourceHandle<::vk::ShaderModule, ShaderCreateInfo>
{
    friend class ObjectPool<Shader>;

public:
    std::string_view getEntryPointName() const
    {
        return getCreateInfo().entrypoint;
    }
    ShaderStage getStage() const
    {
        return getCreateInfo().stage;
    }
    const std::vector<uint32_t>& getCode() const
    {
        return getCreateInfo().code;
    }
    bool hasModule() const
    {
        return getHandle() == VK_NULL_HANDLE;
    }

private:
    Shader(const CreateInfoType& createInfo, HandleType handle);
};

struct ProgramCreateInfo
{
    HashMap<ShaderStage, Shader*> shaders;
};

class ShaderProgram : public ResourceHandle<DummyHandle, ProgramCreateInfo>
{
    friend class ObjectPool<ShaderProgram>;
    friend class Device;

public:
    const VertexInput& getVertexInput() const
    {
        return m_pipelineLayout.vertexInput;
    }
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) const
    {
        if (m_pipelineLayout.setLayouts.size() > setIdx)
        {
            return m_pipelineLayout.setLayouts[setIdx];
        }
        return nullptr;
    }
    Shader* getShader(ShaderStage stage) const
    {
        const auto& shaders = getCreateInfo().shaders;
        if (shaders.contains(stage))
        {
            return shaders.at(stage);
        }
        return nullptr;
    }
    ::vk::ShaderEXT getShaderObject(ShaderStage stage) const
    {
        if (m_shaderObjects.contains(stage))
        {
            return m_shaderObjects.at(stage);
        }
        return VK_NULL_HANDLE;
    }
    ::vk::PipelineLayout getPipelineLayout() const
    {
        return m_pipelineLayout.handle;
    }
    PipelineType getPipelineType() const
    {
        return m_pipelineLayout.type;
    }

    const ::vk::PushConstantRange& getPushConstantRange() const
    {
        return m_pipelineLayout.pushConstantRange;
    }

private:
    ShaderProgram(const CreateInfoType& createInfo, const PipelineLayout& layout,
                  HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps);
    ~ShaderProgram() = default;

private:
    HashMap<ShaderStage, ::vk::ShaderEXT> m_shaderObjects = {};
    PipelineLayout m_pipelineLayout = {};
};

} // namespace aph::vk
