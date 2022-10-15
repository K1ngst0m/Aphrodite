#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "device.h"
#include "vulkan/vulkan_core.h"
#include <unordered_map>

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
    VulkanShaderModule *getShaders(VulkanDevice *device, const std::string &path);
    void                destory(VkDevice device);

private:
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
    EffectInfo                &getInfo();

private:
    VulkanDevice                            *_device;
    std::vector<VulkanDescriptorSetLayout *> _setLayouts;
    VkPipelineLayout                         _pipelineLayout;
    EffectInfo                               _info;
};
} // namespace vkl

#endif // SHADER_H_
