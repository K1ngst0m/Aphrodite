#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "vklDevice.h"
#include "vklInit.hpp"

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

    void buildPipelineLayout(VkDevice device);
    void pushSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    void pushShaderStages(ShaderModule *module, VkShaderStageFlagBits stageBits);
    void pushConstantRanges(VkPushConstantRange constantRange);

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
};
}

#endif // PIPELINE_H_
