#pragma once

#include "allocator/objectPool.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include "forward.h"
#include "vkUtils.h"
#include <bitset>

namespace aph::vk
{

struct PipelineLayoutCreateInfo
{
    VertexInput vertexInput;
    PushConstantRange pushConstantRange = {};
    SmallVector<DescriptorSetLayout*> setLayouts = {};
};

class PipelineLayout : public ResourceHandle<::vk::PipelineLayout, PipelineLayoutCreateInfo>
{
public:
    PipelineLayout(CreateInfoType createInfo, ::vk::PipelineLayout handle)
        : ResourceHandle(handle, createInfo)
    {
    }

    const VertexInput& getVertexInput() noexcept
    {
        return getCreateInfo().vertexInput;
    }

    const PushConstantRange& getPushConstantRange() const noexcept
    {
        return getCreateInfo().pushConstantRange;
    }

    const SmallVector<DescriptorSetLayout*>& getSetLayouts() const noexcept
    {
        return getCreateInfo().setLayouts;
    }

    DescriptorSetLayout* getSetLayout(uint32_t setIdx) const noexcept
    {
        return getCreateInfo().setLayouts.at(setIdx);
    }
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
};

class Shader : public ResourceHandle<DummyHandle, ShaderCreateInfo>
{
    friend class ThreadSafeObjectPool<Shader>;

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

private:
    Shader(const CreateInfoType& createInfo);
};

struct ProgramCreateInfo
{
    HashMap<ShaderStage, Shader*> shaders;
    PipelineLayout* pPipelineLayout = {};
};

class ShaderProgram : public ResourceHandle<DummyHandle, ProgramCreateInfo>
{
    friend class ThreadSafeObjectPool<ShaderProgram>;
    friend class Device;

public:
    const VertexInput& getVertexInput() const
    {
        return getPipelineLayout()->getVertexInput();
    }
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) const
    {
        if (getPipelineLayout()->getSetLayouts().size() > setIdx)
        {
            return getPipelineLayout()->getSetLayout(setIdx);
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
    PipelineLayout* getPipelineLayout() const
    {
        return m_createInfo.pPipelineLayout;
    }

    PipelineType getPipelineType() const
    {
        return m_pipelineType;
    }

    const PushConstantRange& getPushConstantRange() const
    {
        return getPipelineLayout()->getPushConstantRange();
    }

private:
    ShaderProgram(CreateInfoType createInfo, HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps);
    ~ShaderProgram() = default;

private:
    HashMap<ShaderStage, ::vk::ShaderEXT> m_shaderObjects = {};
    PipelineType m_pipelineType;
};

} // namespace aph::vk

namespace aph::vk::utils
{
PipelineType determinePipelineType(const HashMap<ShaderStage, Shader*>& shaders);
}
