#include "shader.h"
#include "pipeline.h"

namespace vkl {
void ShaderEffect::pushSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
    VkDescriptorSetLayout           setLayout;
    VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &perSceneLayoutInfo, nullptr, &setLayout));
    setLayouts.push_back(setLayout);
}
void ShaderEffect::pushConstantRanges(VkPushConstantRange constantRange) {
    constantRanges.push_back(constantRange);
}
void ShaderEffect::pushShaderStages(ShaderModule *module, VkShaderStageFlagBits stageBits) {
    stages.push_back({module, stageBits});
}
void ShaderPass::buildEffect(VkDevice device, VkRenderPass renderPass, PipelineBuilder &builder, vkl::ShaderEffect *shaderEffect) {
    effect = shaderEffect;
    layout = shaderEffect->builtLayout;

    PipelineBuilder pipbuilder = builder;

    pipbuilder.setShaders(shaderEffect);

    builtPipeline = pipbuilder.buildPipeline(device, renderPass);
}

void ShaderEffect::buildPipelineLayout(VkDevice device) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, constantRanges);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &builtLayout));
}
void ShaderCache::destory(VkDevice device) {
    for (auto &[key, shaderModule] : shaderModuleCaches) {
        vkDestroyShaderModule(device, shaderModule.module, nullptr);
    }
}
ShaderModule *ShaderCache::getShaders(const std::shared_ptr<VulkanDevice> &device, const std::string &path) {
    if (!shaderModuleCaches.count(path)) {
        std::vector<char> spvCode      = vkl::utils::loadSpvFromFile(path);
        VkShaderModule    shaderModule = device->createShaderModule(spvCode);

        shaderModuleCaches[path] = {spvCode, shaderModule};
    }
    return &shaderModuleCaches[path];
}
void ShaderEffect::destroy(VkDevice device) {
    for (auto &setLayout : setLayouts) {
        vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    }
    vkDestroyPipelineLayout(device, builtLayout, nullptr);
}
void ShaderPass::destroy(VkDevice device) const {
    vkDestroyPipeline(device, builtPipeline, nullptr);
}
} // namespace vkl
