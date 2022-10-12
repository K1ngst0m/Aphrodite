#include "shader.h"
#include "pipeline.h"
#include "renderpass.h"

namespace vkl {
static VkShaderModule createShaderModule(VulkanDevice            *device,
                                         const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;

    VK_CHECK_RESULT(vkCreateShaderModule(device->getHandle(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

EffectBuilder& EffectBuilder::pushSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings) {

    VkDescriptorSetLayout           setLayout;
    VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device->getHandle(), &perSceneLayoutInfo, nullptr, &setLayout));
    _setLayouts.push_back(setLayout);
    return *this;
}
EffectBuilder& EffectBuilder::pushConstantRanges(VkPushConstantRange constantRange) {
    _constantRanges.push_back(constantRange);
    return *this;
}
EffectBuilder& EffectBuilder::pushShaderStages(VulkanShaderModule *pModule, VkShaderStageFlagBits stageBits) {
    _stages[stageBits] = pModule;
    return *this;
}

void VulkanShaderCache::destory(VkDevice device) {
    for (auto &[key, shaderModule] : shaderModuleCaches) {
        vkDestroyShaderModule(device, shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }
}
VulkanShaderModule *VulkanShaderCache::getShaders(VulkanDevice *device, const std::string &path) {
    if (!shaderModuleCaches.count(path)) {
        std::vector<char> spvCode      = vkl::utils::loadSpvFromFile(path);
        VkShaderModule    shaderModule = createShaderModule(device, spvCode);

        shaderModuleCaches[path] = new VulkanShaderModule(spvCode, shaderModule);
    }
    return shaderModuleCaches[path];
}

void ShaderPass::destroy() const {
    vkDestroyPipeline(_device->getHandle(), builtPipeline, nullptr);
}

VkPipelineLayout ShaderPass::getPipelineLayout() {
    return _effect->getPipelineLayout();
}
std::unique_ptr<ShaderEffect> EffectBuilder::build() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(_setLayouts, _constantRanges);
    VK_CHECK_RESULT(vkCreatePipelineLayout(_device->getHandle(), &pipelineLayoutInfo, nullptr, &_builtLayout));
    auto effect = std::make_unique<ShaderEffect>(_device, _builtLayout, _stages, _constantRanges, _setLayouts);
    return effect;
}
void EffectBuilder::reset() {
    _builtLayout = VK_NULL_HANDLE;
    _stages.clear();
    _constantRanges.clear();
    _setLayouts.clear();
}
EffectBuilder::EffectBuilder(VulkanDevice *device)
    : _device(device) {
}
ShaderPass::ShaderPass(VulkanDevice                 *device,
                       VulkanRenderPass*                  renderPass,
                       PipelineBuilder              &builder,
                       std::shared_ptr<ShaderEffect> effect)
    : _effect(std::move(effect)), _device(device) {
    builder.setShaders(_effect.get());
    builtPipeline = builder.buildPipeline(_device->getHandle(),
                                          renderPass->getHandle());
}
VkPipeline ShaderPass::getPipeline() {
    return builtPipeline;
}
VkDescriptorSetLayout *ShaderPass::getDescriptorSetLayout(uint32_t idx) {
    return _effect->getDescriptorSetLayout(idx);
}
} // namespace vkl
