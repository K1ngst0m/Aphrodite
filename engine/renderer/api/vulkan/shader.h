#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "device.h"

namespace vkl {

class VulkanShaderModule : public ResourceHandle<VkShaderModule> {
public:
    VulkanShaderModule(std::vector<char> code,
                       VkShaderModule    shaderModule,
                       std::string       entrypoint = "main")
        : _entrypoint(std::move(entrypoint)), _code(std::move(code)) {
        _handle = shaderModule;
    }

    std::vector<char> getCode() {
        return _code;
    }

private:
    std::string       _entrypoint;
    std::vector<char> _code;
};

class VulkanShaderCache {
public:
    VulkanShaderCache(VulkanDevice *device);
    VulkanShaderModule *getShaders(const std::string &path);
    void                destory();

private:
    VulkanDevice                                         *_device;
    std::unordered_map<std::string, VulkanShaderModule *> shaderModuleCaches;
};

using ShaderMapList = std::unordered_map<VkShaderStageFlagBits, VulkanShaderModule *>;

struct EffectInfo {
    std::vector<VulkanDescriptorSetLayout *> setLayouts;
    std::vector<VkPushConstantRange>         constants;
    ShaderMapList                            shaderMapList;
};

class ShaderEffect {
public:
    static ShaderEffect *Create(VulkanDevice *pDevice, EffectInfo *pInfo);
    ShaderEffect(VulkanDevice *device);
    ~ShaderEffect();

    VkPipelineLayout           getPipelineLayout();
    VulkanDescriptorSetLayout *getDescriptorSetLayout(uint32_t idx);
    const ShaderMapList       &getShaderMapList() {
        return _shaderMapList;
    }

private:
    VulkanDevice                            *_device;
    std::vector<VkPushConstantRange>         _constants;
    std::vector<VulkanDescriptorSetLayout *> _setLayouts;
    ShaderMapList                            _shaderMapList;
    VkPipelineLayout                         _pipelineLayout;
};

} // namespace vkl

#endif // SHADER_H_
