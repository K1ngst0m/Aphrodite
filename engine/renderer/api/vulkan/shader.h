#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "device.h"

namespace vkl {
class PipelineBuilder;

struct ShaderModule {
    std::vector<char> code;
    VkShaderModule    module;
};

struct ShaderParameters {
};

/**
 * @brief holds all of the shader related state that a pipeline needs to be built.
 */
struct ShaderEffect {
    VkPipelineLayout                   builtLayout;
    std::vector<VkPushConstantRange>   constantRanges;
    std::vector<VkDescriptorSetLayout> setLayouts;

    struct ShaderStage {
        ShaderModule         *shaderModule;
        VkShaderStageFlagBits stage;
    };

    std::vector<ShaderStage> stages;

    void pushSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    void pushShaderStages(ShaderModule *module, VkShaderStageFlagBits stageBits);
    void pushConstantRanges(VkPushConstantRange constantRange);

    void buildPipelineLayout(VkDevice device);

    void destroy(VkDevice device);
};

struct ShaderCache {
    std::unordered_map<std::string, ShaderModule> shaderModuleCaches;
    ShaderModule                                 *getShaders(VulkanDevice *device, const std::string &path);
    void                                          destory(VkDevice device);
};

struct ShaderPass {
    ShaderEffect    *effect        = nullptr;
    VkPipeline       builtPipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout        = VK_NULL_HANDLE;

    void buildEffect(VkDevice device, VkRenderPass renderPass, PipelineBuilder &builder, vkl::ShaderEffect *effect);

    void destroy(VkDevice device) const;
};

struct EffectTemplate {
    std::vector<std::shared_ptr<ShaderPass>> passShaders;
    ShaderParameters                        *defaultParameters;
};

} // namespace vkl

#endif // SHADER_H_
