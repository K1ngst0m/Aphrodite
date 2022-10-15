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
ShaderEffect::ShaderEffect(VulkanDevice *device)
    : _device(device) {
}
VkPipelineLayout ShaderEffect::getPipelineLayout() {
    return _pipelineLayout;
}
VkDescriptorSetLayout *ShaderEffect::getDescriptorSetLayout(uint32_t idx) {
    return &_setLayouts[idx];
}
ShaderEffect::~ShaderEffect() {
    for (auto &setLayout : _setLayouts) {
        vkDestroyDescriptorSetLayout(_device->getHandle(), setLayout, nullptr);
    }
    vkDestroyPipelineLayout(_device->getHandle(), _pipelineLayout, nullptr);
}
ShaderEffect *ShaderEffect::Create(VulkanDevice *pDevice, EffectInfo *pInfo) {
    auto instance = new ShaderEffect(pDevice);
    for (auto &layoutDesc : pInfo->setLayouts){
        VkDescriptorSetLayout           setLayout;
        VkDescriptorSetLayoutCreateInfo layoutInfo = vkl::init::descriptorSetLayoutCreateInfo(layoutDesc.bindings);
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(pDevice->getHandle(), &layoutInfo, nullptr, &setLayout));
        instance->_setLayouts.push_back(setLayout);
    }
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(instance->_setLayouts, pInfo->constants);
    VK_CHECK_RESULT(vkCreatePipelineLayout(pDevice->getHandle(), &pipelineLayoutInfo, nullptr, &instance->_pipelineLayout));
    memcpy(&instance->_info, pInfo, sizeof(EffectInfo));
    return instance;
}
EffectInfo &ShaderEffect::getInfo() {
    return _info;
}
} // namespace vkl
