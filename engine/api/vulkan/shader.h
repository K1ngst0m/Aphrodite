#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Sampler;
class ImmutableSampler;
class DescriptorSetLayout;
class DescriptorSet;

struct ShaderLayout
{
    uint32_t sampledImageMask               = 0;
    uint32_t storageImageMask               = 0;
    uint32_t uniformBufferMask              = 0;
    uint32_t storageBufferMask              = 0;
    uint32_t sampledTexelBufferMask         = 0;
    uint32_t storageTexelBufferMask         = 0;
    uint32_t inputAttachmentMask            = 0;
    uint32_t samplerMask                    = 0;
    uint32_t separateImageMask              = 0;
    uint32_t fpMask                         = 0;
    uint32_t immutableSamplerMask           = 0;
    uint8_t  arraySize[VULKAN_NUM_BINDINGS] = {};
    uint32_t padding                        = 0;
    enum
    {
        UNSIZED_ARRAY = 0xff
    };
};

struct ResourceLayout
{
    ShaderLayout setShaderLayouts[VULKAN_NUM_DESCRIPTOR_SETS] = {};

    uint32_t inputMask        = {};
    uint32_t outputMask       = {};
    uint32_t pushConstantSize = {};
    uint32_t specConstantMask = {};
    uint32_t bindlessSetMask  = {};
};

struct CombinedResourceLayout
{
    struct SetInfo
    {
        ShaderLayout shaderLayout                           = {};
        uint32_t     stagesForBindings[VULKAN_NUM_BINDINGS] = {};
        uint32_t     stagesForSets                          = {};
    };
    SetInfo setInfos[VULKAN_NUM_DESCRIPTOR_SETS] = {};

    VkPushConstantRange pushConstantRange = {};

    uint32_t attributeMask             = {};
    uint32_t renderTargetMask          = {};
    uint32_t descriptorSetMask         = {};
    uint32_t bindlessDescriptorSetMask = {};
    uint32_t combinedSpecConstantMask  = {};

    std::unordered_map<ShaderStage, uint32_t> specConstantMask = {};
};

struct ImmutableSamplerBank
{
    const Sampler* samplers[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
};

class Shader : public ResourceHandle<VkShaderModule>
{
    friend class ShaderProgram;

public:
    Shader(std::vector<uint32_t> code, HandleType handle, std::string entrypoint = "main",
           const ResourceLayout* pLayout = nullptr);

private:
    static ResourceLayout ReflectLayout(const std::vector<uint32_t>& code);
    std::string           m_entrypoint = {};
    std::vector<uint32_t> m_code       = {};
    ResourceLayout        m_layout     = {};
};
using ShaderMapList = std::unordered_map<ShaderStage, Shader*>;

class ShaderProgram
{
public:
    ShaderProgram(Device* device, Shader* vs, Shader* fs, const ImmutableSamplerBank* samplerBank);
    ShaderProgram(Device* device, Shader* cs, const ImmutableSamplerBank* samplerBank);

    ~ShaderProgram();

    VkShaderStageFlags getConstantShaderStage(uint32_t offset, uint32_t size) const;

    DescriptorSetLayout* getSetLayout(uint32_t setIdx) { return m_pSetLayouts[setIdx]; }
    ShaderMapList        getShaders() const { return m_shaders; }
    Shader*              getShader(ShaderStage stage) { return m_shaders[stage]; }
    VkPipelineLayout     getPipelineLayout() const { return m_pipeLayout; }

private:
    void createPipelineLayout(const ImmutableSamplerBank* samplerBank);
    void combineLayout(const ImmutableSamplerBank* samplerBank);

private:
    Device*                           m_pDevice       = {};
    ShaderMapList                     m_shaders       = {};
    std::vector<DescriptorSetLayout*> m_pSetLayouts   = {};
    VkPipelineLayout                  m_pipeLayout    = {};
    CombinedResourceLayout            m_combineLayout = {};
    std::vector<VkDescriptorPoolSize> m_poolSize      = {};
};

}  // namespace aph::vk

#endif  // SHADER_H_
