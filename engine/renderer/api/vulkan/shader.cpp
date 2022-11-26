#include "shader.h"
#include "pipeline.h"
#include "renderpass.h"
#include "descriptorSetLayout.h"

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

void VulkanShaderCache::destory() {
    for (auto &[key, shaderModule] : shaderModuleCaches) {
        vkDestroyShaderModule(_device->getHandle(), shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }
}
VulkanShaderModule *VulkanShaderCache::getShaders(const std::string &path) {
    if (!shaderModuleCaches.count(path)) {
        std::vector<char> spvCode      = vkl::utils::loadSpvFromFile(path);
        VkShaderModule    shaderModule = createShaderModule(_device, spvCode);

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
VulkanDescriptorSetLayout *ShaderEffect::getDescriptorSetLayout(uint32_t idx) {
    return _setLayouts[idx];
}
ShaderEffect::~ShaderEffect() {
    vkDestroyPipelineLayout(_device->getHandle(), _pipelineLayout, nullptr);
}
ShaderEffect *ShaderEffect::Create(VulkanDevice *pDevice, EffectInfo *pInfo) {
    auto instance = new ShaderEffect(pDevice);
    std::vector<VkDescriptorSetLayout> setLayouts;
    for (auto setLayout : pInfo->setLayouts){
        instance->_setLayouts.push_back(setLayout);
        setLayouts.push_back(setLayout->getHandle());
    }
    instance->_constants = pInfo->constants;
    instance->_shaderMapList = pInfo->shaderMapList;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, pInfo->constants);
    VK_CHECK_RESULT(vkCreatePipelineLayout(pDevice->getHandle(), &pipelineLayoutInfo, nullptr, &instance->_pipelineLayout));
    return instance;
}
VulkanShaderCache::VulkanShaderCache(VulkanDevice *device)
    : _device(device) {
}
const ShaderMapList &ShaderEffect::getShaderMapList() {
    return _shaderMapList;
}
} // namespace vkl
