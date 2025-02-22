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
    ShaderLayout      shaderLayouts[VULKAN_NUM_DESCRIPTOR_SETS]   = {};
    VertexAttribState vertexAttributes[VULKAN_NUM_VERTEX_ATTRIBS] = {};

    std::bitset<32> inputMask        = {};
    std::bitset<32> outputMask       = {};
    std::bitset<32> specConstantMask = {};
    std::bitset<32> bindlessSetMask  = {};
    uint32_t pushConstantSize = {};
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

    std::bitset<32> attributeMask             = {};
    std::bitset<32> renderTargetMask          = {};
    std::bitset<32> descriptorSetMask         = {};
    std::bitset<32> bindlessDescriptorSetMask = {};
    std::bitset<32> combinedSpecConstantMask  = {};

    HashMap<ShaderStage, std::bitset<32>> specConstantMask = {};
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
};

class Shader : public ResourceHandle<VkShaderModule, ShaderCreateInfo>
{
    friend class ObjectPool<Shader>;

public:
    std::string_view      getEntryPointName() const { return getCreateInfo().entrypoint; }
    const ResourceLayout& getResourceLayout() const { return m_layout; }
    ShaderStage           getStage() const { return getCreateInfo().stage; }

private:
    Shader(const CreateInfoType& createInfo, HandleType handle, ResourceLayout layout);
    ResourceLayout m_layout;
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

    PipelineType type = {};

    ImmutableSamplerBank samplerBank = {};
};

class ShaderProgram : public ResourceHandle<DummyHandle, ProgramCreateInfo>
{
    friend class ObjectPool<ShaderProgram>;
    friend class Device;

public:
    VkShaderStageFlags getConstantShaderStage(uint32_t offset, uint32_t size) const;

    const VertexInput&   getVertexInput() const { return m_vertexInput; }
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) { return m_pSetLayouts[setIdx]; }
    Shader*              getShader(ShaderStage stage) { return m_shaders[stage]; }
    VkPipelineLayout     getPipelineLayout() const { return m_pipeLayout; }
    PipelineType         getPipelineType() const { return getCreateInfo().type; }

private:
    ShaderProgram(const CreateInfoType& createInfo, const CombinedResourceLayout& layout,
                  VkPipelineLayout pipelineLayout, SmallVector<DescriptorSetLayout*> setLayouts);
    ~ShaderProgram() = default;

    void createVertexInput();

private:
    ShaderMapList                     m_shaders       = {};
    SmallVector<DescriptorSetLayout*> m_pSetLayouts   = {};
    VkPipelineLayout                  m_pipeLayout    = {};
    CombinedResourceLayout            m_combineLayout = {};
    VertexInput                       m_vertexInput   = {};
};

}  // namespace aph::vk

#endif  // SHADER_H_
