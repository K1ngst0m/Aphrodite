#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

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

struct VertexAttribState
{
    uint32_t binding;
    VkFormat format;
    uint32_t offset;
    uint32_t size;
};

struct ResourceLayout
{
    ShaderLayout      setShaderLayouts[VULKAN_NUM_DESCRIPTOR_SETS] = {};
    VertexAttribState vertexAttr[VULKAN_NUM_VERTEX_ATTRIBS]        = {};

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

    VertexAttribState   vertexAttr[VULKAN_NUM_VERTEX_ATTRIBS] = {};
    VkPushConstantRange pushConstantRange                     = {};

    uint32_t attributeMask             = {};
    uint32_t renderTargetMask          = {};
    uint32_t descriptorSetMask         = {};
    uint32_t bindlessDescriptorSetMask = {};
    uint32_t combinedSpecConstantMask  = {};

    HashMap<ShaderStage, uint32_t> specConstantMask = {};
};

struct ImmutableSamplerBank
{
    const Sampler* samplers[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
};

class Shader : public ResourceHandle<VkShaderModule>
{
    friend class ShaderProgram;
    friend class ObjectPool<Shader>;

public:
    std::string_view getEntryPointName() const { return m_entrypoint; }

private:
    Shader(ResourceLayout layout, HandleType handle, std::string entrypoint = "main");
    std::string    m_entrypoint = {};
    ResourceLayout m_layout     = {};
};
using ShaderMapList = HashMap<ShaderStage, Shader*>;

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

    PipelineType          type        = {};
    ImmutableSamplerBank* samplerBank = {};
};

class ShaderProgram
{
    friend class ObjectPool<ShaderProgram>;

public:
    VkShaderStageFlags getConstantShaderStage(uint32_t offset, uint32_t size) const;

    const VertexInput&   getVertexInput() const { return m_vertexInput; }
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) { return m_pSetLayouts[setIdx]; }
    const ShaderMapList& getShaders() const { return m_shaders; }
    Shader*              getShader(ShaderStage stage) { return m_shaders[stage]; }
    VkPipelineLayout     getPipelineLayout() const { return m_pipeLayout; }
    PipelineType         getPipelineType() const { return m_pipelineType; }

private:
    ShaderProgram(Device* device, Shader* ms, Shader* ts, Shader* fs, const ImmutableSamplerBank* samplerBank);
    ShaderProgram(Device* device, Shader* vs, Shader* fs, const ImmutableSamplerBank* samplerBank);
    ShaderProgram(Device* device, Shader* cs, const ImmutableSamplerBank* samplerBank);
    ~ShaderProgram();

    void createPipelineLayout(const ImmutableSamplerBank* samplerBank);
    void createVertexInput();
    void combineLayout(const ImmutableSamplerBank* samplerBank);

private:
    Device*                           m_pDevice       = {};
    ShaderMapList                     m_shaders       = {};
    SmallVector<DescriptorSetLayout*> m_pSetLayouts   = {};
    VkPipelineLayout                  m_pipeLayout    = {};
    CombinedResourceLayout            m_combineLayout = {};
    SmallVector<VkDescriptorPoolSize> m_poolSize      = {};
    VertexInput                       m_vertexInput   = {};
    PipelineType                      m_pipelineType  = {};
};

}  // namespace aph::vk

#endif  // SHADER_H_
