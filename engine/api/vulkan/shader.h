#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include <bitset>
#include "common/hash.h"
#include "common/smallVector.h"
#include "allocator/objectPool.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Sampler;
class ImmutableSampler;
class DescriptorSetLayout;
class DescriptorSet;

struct PipelineLayout
{
    VertexInput         vertexInput       = {};
    VkPushConstantRange pushConstantRange = {};

    SmallVector<DescriptorSetLayout*> setLayouts = {};
    VkPipelineLayout                  handle     = {};
};

struct ImmutableSamplerBank
{
    Sampler* samplers[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
};

struct ShaderCreateInfo
{
    std::vector<uint32_t> code;
    std::string           entrypoint = "main";
    aph::ShaderStage      stage;
    bool                  compile = false;
};

class Shader : public ResourceHandle<VkShaderModule, ShaderCreateInfo>
{
    friend class ObjectPool<Shader>;

public:
    std::string_view             getEntryPointName() const { return getCreateInfo().entrypoint; }
    ShaderStage                  getStage() const { return getCreateInfo().stage; }
    const std::vector<uint32_t>& getCode() const { return getCreateInfo().code; }
    bool                         hasModule() const { return getHandle() == VK_NULL_HANDLE; }

private:
    Shader(const CreateInfoType& createInfo, HandleType handle);
};

struct ProgramCreateInfo
{
    union
    {
        struct
        {
            Shader* pMesh     = {};
            Shader* pTask     = {};
            Shader* pFragment = {};
        } mesh;

        struct
        {
            Shader* pCompute = {};
        } compute;

        struct
        {
            Shader* pVertex   = {};
            Shader* pFragment = {};
        } geometry;
    };

    PipelineType type = {};

    ImmutableSamplerBank samplerBank = {};
};

class ShaderProgram : public ResourceHandle<DummyHandle, ProgramCreateInfo>
{
    friend class ObjectPool<ShaderProgram>;
    friend class Device;

public:
    const VertexInput&   getVertexInput() const { return m_pipelineLayout.vertexInput; }
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) const
    {
        if(m_pipelineLayout.setLayouts.size() > setIdx)
        {
            return m_pipelineLayout.setLayouts[setIdx];
        }
        return nullptr;
    }
    Shader* getShader(ShaderStage stage) const
    {
        if(m_shaders.contains(stage))
        {
            return m_shaders.at(stage);
        }
        return nullptr;
    }
    VkShaderEXT getShaderObject(ShaderStage stage) const
    {
        if(m_shaderObjects.contains(stage))
        {
            return m_shaderObjects.at(stage);
        }
        return VK_NULL_HANDLE;
    }
    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout.handle; }
    PipelineType     getPipelineType() const { return getCreateInfo().type; }

    const VkPushConstantRange& getPushConstantRange() const { return m_pipelineLayout.pushConstantRange; }

private:
    ShaderProgram(const CreateInfoType& createInfo, const PipelineLayout& layout,
                  HashMap<ShaderStage, VkShaderEXT> shaderObjectMaps);
    ~ShaderProgram() = default;

private:
    HashMap<ShaderStage, Shader*>     m_shaders        = {};
    HashMap<ShaderStage, VkShaderEXT> m_shaderObjects  = {};
    PipelineLayout                    m_pipelineLayout = {};
};

}  // namespace aph::vk

#endif  // SHADER_H_
