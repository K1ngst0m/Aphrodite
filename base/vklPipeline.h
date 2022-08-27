#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "vklDevice.h"
#include "vklInit.hpp"

namespace vkl {
/**
 * @brief holds all of the shader related state that a pipeline needs to be built.
 */
struct ShaderEffect {
    VkPipelineLayout pipelineLayout;
    std::vector<VkPushConstantRange> constantRanges;
    std::vector<VkDescriptorSetLayout> setLayouts;

    struct ShaderStage {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
    };

    std::vector<ShaderStage> stages;

    struct{
        uint32_t constantCount = 0;
        uint32_t setCount = 0;
    }reflectData;

    // TODO reflect to combined shader
    void build(vkl::Device *device, const std::string &combinedCodePath);
    void build(vkl::Device *device, const std::string &vertCodePath, const std::string &fragCodePath);

    void pushSetLayout(vkl::Device *device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    void pushConstantRanges(VkPushConstantRange constantRange);

    void reflectToPipelineLayout(vkl::Device *device, const void *spirv_code, size_t spirv_nbytes);
};

/**
 * @brief built version of a Shader Effect, where it stores the built pipeline
 */
struct ShaderPass {
    ShaderEffect* effect = nullptr;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void build(vkl::ShaderEffect effect, VkPipeline pipeline){
        this->effect = &effect;
        this->pipeline = pipeline;
        this->layout = effect.pipelineLayout;
    }
};

enum class VertexAttributeTemplate {
    DefaultVertex,
    DefaultVertexPosOnly
};

struct EffectBuilder{
    VertexAttributeTemplate vertexAttrib;
    ShaderEffect* effect{ nullptr };

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

    void setShaders(const ShaderEffect &shaders);
};
}

#endif // PIPELINE_H_
