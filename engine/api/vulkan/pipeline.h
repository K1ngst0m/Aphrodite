#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"
#include "vkInit.hpp"

namespace vkl {

enum class VertexComponent {
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct VertexInputBuilder{
    static VkVertexInputBindingDescription                _vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;
    static VkVertexInputAttributeDescription              inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);
    static void                                           setPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

struct ShaderModule{
    std::vector<char> code;
    VkShaderModule module;
};

struct ShaderCache{
    std::unordered_map<std::string, ShaderModule> shaderModuleCaches;

    ShaderModule * getShaders(vkl::VulkanDevice * device, const std::string & path){
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
    void resetToDefault(VkExtent2D extent);
};
}

#endif // PIPELINE_H_
