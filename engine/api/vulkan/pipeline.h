#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"
#include "mesh.h"
#include "vkInit.hpp"

namespace vkl {

struct ShaderModule{
    std::vector<char> code;
    VkShaderModule module;
};

struct ShaderCache{
    std::unordered_map<std::string, ShaderModule> shaderModuleCaches;

    ShaderModule * getShaders(vkl::Device * device, const std::string & path){
        if (!shaderModuleCaches.count(path)){
            std::vector<char> spvCode = vkl::utils::loadSpvFromFile(path);
            VkShaderModule shaderModule = device->createShaderModule(spvCode);

            shaderModuleCaches[path] = {spvCode, shaderModule};
        }

        return &shaderModuleCaches[path];
    }

    void destory(VkDevice device){
        for (auto & [key, shaderModule]: shaderModuleCaches){
            vkDestroyShaderModule(device, shaderModule.module, nullptr);
        }
    }
};

/**
 * @brief holds all of the shader related state that a pipeline needs to be built.
 */
struct ShaderEffect {
    VkPipelineLayout builtLayout;
    std::vector<VkPushConstantRange> constantRanges;
    std::vector<VkDescriptorSetLayout> setLayouts;

    struct ShaderStage {
        ShaderModule * shaderModule;
        VkShaderStageFlagBits stage;
    };

    std::vector<ShaderStage> stages;

    ShaderEffect& pushSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    ShaderEffect& pushShaderStages(ShaderModule *module, VkShaderStageFlagBits stageBits);
    ShaderEffect& pushConstantRanges(VkPushConstantRange constantRange);

    ShaderEffect& buildPipelineLayout(VkDevice device);

    void destroy(VkDevice device){
        for (auto & setLayout: setLayouts){
            vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
        }
        vkDestroyPipelineLayout(device, builtLayout, nullptr);
    }
};

class PipelineBuilder;
/**
 * @brief built version of a Shader Effect, where it stores the built pipeline
 */
struct ShaderPass {
    ShaderEffect* effect = nullptr;
    VkPipeline builtPipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void build(VkDevice device, VkRenderPass renderPass, PipelineBuilder &builder, vkl::ShaderEffect *effect);

    void destroy(VkDevice device) const{
        vkDestroyPipeline(device, builtPipeline, nullptr);
    }
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

    void setShaders(ShaderEffect *shaders);
    void resetToDefault(VkExtent2D extent){
        vkl::VertexLayout::setPipelineVertexInputState({vkl::VertexComponent::POSITION, vkl::VertexComponent::NORMAL,
                                                        vkl::VertexComponent::UV, vkl::VertexComponent::COLOR, vkl::VertexComponent::TANGENT});
        _vertexInputInfo = vkl::VertexLayout::_pipelineVertexInputStateCreateInfo;
        _inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        _viewport = vkl::init::viewport(extent);
        _scissor = vkl::init::rect2D(extent);

        _dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        _dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(_dynamicStages.data(), static_cast<uint32_t>(_dynamicStages.size()));

        _rasterizer = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        _multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        _colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        _depthStencil = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    }
};
}

#endif // PIPELINE_H_
