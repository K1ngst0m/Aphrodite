#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class ImmutableSampler;
class DescriptorSetLayout;

constexpr unsigned VULKAN_NUM_DESCRIPTOR_SETS           = 4;
constexpr unsigned VULKAN_NUM_BINDINGS                  = 32;
constexpr unsigned VULKAN_NUM_BINDINGS_BINDLESS_VARYING = 16 * 1024;
constexpr unsigned VULKAN_NUM_ATTACHMENTS               = 8;
constexpr unsigned VULKAN_NUM_VERTEX_ATTRIBS            = 16;
constexpr unsigned VULKAN_NUM_VERTEX_BUFFERS            = 4;
constexpr unsigned VULKAN_PUSH_CONSTANT_SIZE            = 128;
constexpr unsigned VULKAN_MAX_UBO_SIZE                  = 16 * 1024;
constexpr unsigned VULKAN_NUM_USER_SPEC_CONSTANTS       = 8;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS   = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL    = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;

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
    const ImmutableSampler* samplers[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
};

class Shader : public ResourceHandle<VkShaderModule>
{
    friend class ShaderProgram;

public:
    static std::unique_ptr<Shader> Create(Device* pDevice, const std::filesystem::path& path,
                                          const std::string&    entrypoint = "main",
                                          const ResourceLayout* pLayout    = nullptr);

private:
    static ResourceLayout ReflectLayout(const std::vector<uint32_t>& code);
    Shader(std::vector<uint32_t> code, VkShaderModule shaderModule, std::string entrypoint = "main",
           const ResourceLayout* pLayout = nullptr);
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

public:
    void                 createPipelineLayout(const ImmutableSamplerBank* samplerBank);
    void                 combineLayout(const ImmutableSamplerBank* samplerBank);
    DescriptorSetLayout* getSetLayout(uint32_t setIdx) { return m_pSetLayouts[setIdx]; }

public:
    void                              createUpdateTemplates();
    Device*                           m_pDevice       = {};
    ShaderMapList                     m_shaders       = {};
    std::vector<DescriptorSetLayout*> m_pSetLayouts   = {};
    VkPipelineLayout                  m_pipeLayout    = {};
    CombinedResourceLayout            m_combineLayout = {};
    std::vector<VkDescriptorPoolSize> m_poolSize      = {};
};

}  // namespace aph::vk

#endif  // SHADER_H_
