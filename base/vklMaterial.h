#ifndef VKLMATERIAL_H_
#define VKLMATERIAL_H_

#include "vklUtils.h"
#include "vklTexture.h"
#include "vklInit.hpp"

namespace vkl {
struct Material {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex = 0;
    glm::vec4 specularFactor = glm::vec4(1.0f);
    float shininess = 64.0f;

    vkl::Texture *baseColorTexture = nullptr;
    vkl::Texture *specularTexture = nullptr;

    VkDescriptorSet descriptorSet;

    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
};

}

namespace vklt
{

enum class DescriptorSetTypes: uint8_t{
    SCENE,
    MATERIAL,
    COUNT,
};

/**
 * @brief holds all of the shader related state that a pipeline needs to be built.
 */
struct ShaderEffect {
    VkPipelineLayout pipelineLayout;
    std::array<VkDescriptorSetLayout, static_cast<uint32_t>(DescriptorSetTypes::COUNT)> setLayouts;

    struct ShaderStage {
        VkShaderModule *shaderModule;
        VkShaderStageFlagBits stage;
    };

    std::vector<ShaderStage> stages;
};

/**
 * @brief built version of a Shader Effect, where it stores the built pipeline
 */
struct ShaderPass {
    ShaderEffect* effect = nullptr;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};

enum class VertexAttributeTemplate {
    DefaultVertex,
    DefaultVertexPosOnly
};

struct EffectBuilder{
    VertexAttributeTemplate vertexAttrib;
    struct ShaderEffect* effect{ nullptr };

    VkPrimitiveTopology topology;
    VkPipelineRasterizationStateCreateInfo rasterizerInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
};

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    std::vector<VkDynamicState> _dynamicStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineDynamicStateCreateInfo _dynamicState;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineLayout _pipelineLayout;

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);

    void setShaders(const vklt::ShaderEffect &shaders);
};

}


#endif // VKLMATERIAL_H_
