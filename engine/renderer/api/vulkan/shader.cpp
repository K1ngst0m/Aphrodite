#include "shader.h"
#include "pipeline.h"

namespace vkl {
static VkShaderModule createShaderModule(VulkanDevice* device,
                                         const std::vector<char>             &code) {
    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;

    VK_CHECK_RESULT(vkCreateShaderModule(device->getHandle(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

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
ShaderModule *ShaderCache::getShaders(VulkanDevice *device, const std::string &path) {
    if (!shaderModuleCaches.count(path)) {
        std::vector<char> spvCode      = vkl::utils::loadSpvFromFile(path);
        VkShaderModule    shaderModule = createShaderModule(device, spvCode);

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
