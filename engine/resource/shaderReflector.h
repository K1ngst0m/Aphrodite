#pragma once

#include "api/vulkan/shader.h"

namespace aph
{
struct ShaderLayout
{
    std::bitset<32> sampledImageMask = {};
    std::bitset<32> storageImageMask = {};
    std::bitset<32> uniformBufferMask = {};
    std::bitset<32> storageBufferMask = {};
    std::bitset<32> sampledTexelBufferMask = {};
    std::bitset<32> storageTexelBufferMask = {};
    std::bitset<32> inputAttachmentMask = {};
    std::bitset<32> samplerMask = {};
    std::bitset<32> separateImageMask = {};
    std::bitset<32> fpMask = {};
    std::bitset<32> immutableSamplerMask = {};

    uint8_t arraySize[VULKAN_NUM_BINDINGS] = {};
    uint32_t padding = 0;

    enum
    {
        UNSIZED_ARRAY = 0xff
    };
};

struct VertexAttribState
{
    uint32_t binding;
    Format format;
    uint32_t offset;
    uint32_t size;
};

struct ResourceLayout
{
    ShaderLayout layouts[VULKAN_NUM_DESCRIPTOR_SETS] = {};
    VertexAttribState vertexAttributes[VULKAN_NUM_VERTEX_ATTRIBS] = {};

    std::bitset<32> inputMask = {};
    std::bitset<32> outputMask = {};
    std::bitset<32> specConstantMask = {};
    std::bitset<32> bindlessSetMask = {};
    uint32_t pushConstantSize = {};
};

struct CombinedResourceLayout
{
    struct SetInfo
    {
        ShaderLayout shaderLayout = {};
        ::vk::ShaderStageFlags stagesForBindings[VULKAN_NUM_BINDINGS] = {};
        ::vk::ShaderStageFlags stagesForSets = {};
    };
    std::array<SetInfo, VULKAN_NUM_DESCRIPTOR_SETS> setInfos = {};
    std::array<VertexAttribState, VULKAN_NUM_VERTEX_ATTRIBS> vertexAttr = {};

    ::vk::PushConstantRange pushConstantRange = {};

    std::bitset<32> attributeMask = {};
    std::bitset<32> renderTargetMask = {};
    std::bitset<32> descriptorSetMask = {};
    std::bitset<32> bindlessDescriptorSetMask = {};
    std::bitset<32> combinedSpecConstantMask = {};

    HashMap<ShaderStage, std::bitset<32>> specConstantMask = {};
};

struct ReflectRequest
{
    std::vector<vk::Shader*> shaders;
    const vk::ImmutableSamplerBank* samplerBank = {};
};

class ShaderReflector
{
public:
    ShaderReflector(ReflectRequest request);

    ::vk::PushConstantRange getPushConstantRange() const
    {
        return m_combinedLayout.pushConstantRange;
    }
    VertexInput getVertexInput() const
    {
        return m_vertexInput;
    }
    CombinedResourceLayout getReflectLayoutMeta() const
    {
        return m_combinedLayout;
    }
    SmallVector<::vk::DescriptorSetLayoutBinding> getLayoutBindings(uint32_t set);
    SmallVector<::vk::DescriptorPoolSize> getPoolSizes(uint32_t set);

private:
    void reflect();
    ResourceLayout reflectStageLayout(const std::vector<uint32_t>& spvCode);

    VertexInput m_vertexInput;
    ReflectRequest m_request;
    CombinedResourceLayout m_combinedLayout;
    HashMap<ShaderStage, ResourceLayout> m_stageLayouts;

    struct SetInfo
    {
        SmallVector<::vk::DescriptorSetLayoutBinding> bindings;
        SmallVector<::vk::DescriptorPoolSize> poolSizes;
    };
    SetInfo setInfos[VULKAN_NUM_DESCRIPTOR_SETS] = {};
};
} // namespace aph
