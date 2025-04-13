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
    PushConstantRange pushConstantRange          = {};
    SmallVector<DescriptorSetLayout*> setLayouts = {};
};

class PipelineLayout : public ResourceHandle<::vk::PipelineLayout, PipelineLayoutCreateInfo>
{
public:
    PipelineLayout(CreateInfoType createInfo, ::vk::PipelineLayout handle);

    auto getVertexInput() noexcept -> const VertexInput&;
    auto getPushConstantRange() const noexcept -> const PushConstantRange&;
    auto getSetLayouts() const noexcept -> const SmallVector<DescriptorSetLayout*>&;
    auto getSetLayout(uint32_t setIdx) const noexcept -> DescriptorSetLayout*;
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
    auto getEntryPointName() const -> std::string_view;
    auto getStage() const -> ShaderStage;
    auto getCode() const -> const std::vector<uint32_t>&;

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
    auto getVertexInput() const -> const VertexInput&;
    auto getSetLayout(uint32_t setIdx) const -> DescriptorSetLayout*;
    auto getShader(ShaderStage stage) const -> Shader*;
    auto getShaderObject(ShaderStage stage) const -> ::vk::ShaderEXT;
    auto getPipelineLayout() const -> PipelineLayout*;
    auto getPipelineType() const -> PipelineType;
    auto getPushConstantRange() const -> const PushConstantRange&;

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
