#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "device.h"
#include "vulkan/vulkan_core.h"
#include <unordered_map>

namespace vkl {

enum class ShaderStage{
    VS,
    FS,
    CS,
};

class VulkanShaderModule : public ResourceHandle<VkShaderModule> {
public:
    VulkanShaderModule(std::vector<char> code,
                       VkShaderModule shaderModule,
                       std::string entrypoint = "main")
        : _entrypoint(std::move(entrypoint)), _code(std::move(code)) {
        _handle = shaderModule;
    }

    std::vector<char> getCode() {
        return _code;
    }

private:
    std::string _entrypoint;
    std::vector<char> _code;
    ShaderStage stage;
};

class VulkanShaderCache {
public:
    VulkanShaderModule *getShaders(VulkanDevice *device, const std::string &path);
    void                destory(VkDevice device);

private:
    std::unordered_map<std::string, VulkanShaderModule *> shaderModuleCaches;
    std::unordered_map<ShaderStage, VkShaderStageFlags> stageMaps{
        {ShaderStage::VS, VK_SHADER_STAGE_VERTEX_BIT},
        {ShaderStage::FS, VK_SHADER_STAGE_FRAGMENT_BIT},
        {ShaderStage::CS, VK_SHADER_STAGE_COMPUTE_BIT},
    };
};

using ShaderMapList = std::unordered_map<VkShaderStageFlagBits, VulkanShaderModule *>;

class ShaderEffect {
public:
    ShaderEffect(VulkanDevice                      *device,
                 VkPipelineLayout                   pipelineLayout,
                 ShaderMapList                      shaderMapList,
                 std::vector<VkPushConstantRange>   constantRanges,
                 std::vector<VkDescriptorSetLayout> setLayouts)
        : _device(device),
          _builtLayout(pipelineLayout),
          _stages(std::move(shaderMapList)),
          _constantRanges(std::move(constantRanges)),
          _setLayouts(std::move(setLayouts)) {
    }

    void destroy() {
        for (auto &setLayout : _setLayouts) {
            vkDestroyDescriptorSetLayout(_device->getHandle(), setLayout, nullptr);
        }
        vkDestroyPipelineLayout(_device->getHandle(), _builtLayout, nullptr);
    }

    VkPipelineLayout getPipelineLayout() {
        return _builtLayout;
    }

    VkDescriptorSetLayout *getDescriptorSetLayout(uint32_t idx) {
        return &_setLayouts[idx];
    }

    ShaderMapList& getStages(){
        return _stages;
    }

private:
    VulkanDevice                      *_device;
    VkPipelineLayout                   _builtLayout;
    ShaderMapList                      _stages;
    std::vector<VkPushConstantRange>   _constantRanges;
    std::vector<VkDescriptorSetLayout> _setLayouts;
};

class EffectBuilder {
public:
    EffectBuilder(VulkanDevice *device);
    EffectBuilder                &pushSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    EffectBuilder                &pushShaderStages(VulkanShaderModule *pModule, VkShaderStageFlagBits stageBits);
    EffectBuilder                &pushConstantRanges(VkPushConstantRange constantRange);
    std::unique_ptr<ShaderEffect> build();

    void reset();

private:
    VulkanDevice                      *_device;
    VkPipelineLayout                   _builtLayout;
    ShaderMapList                      _stages;
    std::vector<VkPushConstantRange>   _constantRanges;
    std::vector<VkDescriptorSetLayout> _setLayouts;
    std::set<ShaderEffect *>           _effects;
};

} // namespace vkl

#endif // SHADER_H_
