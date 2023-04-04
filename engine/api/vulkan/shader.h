#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;

class VulkanShaderModule : public ResourceHandle<VkShaderModule>
{
public:
    VulkanShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint = "main");

    std::vector<char> getCode() { return _code; }

private:
    std::string _entrypoint;
    std::vector<char> _code;
};

class VulkanShaderCache
{
public:
    VulkanShaderCache(VulkanDevice *device) : _device(device) {}
    VulkanShaderModule *getShaders(const std::string &path);
    void destroy();

private:
    VulkanDevice *_device;
    std::unordered_map<std::string, VulkanShaderModule *> shaderModuleCaches;
};

using ShaderMapList = std::unordered_map<VkShaderStageFlagBits, VulkanShaderModule *>;

}  // namespace vkl

#endif  // SHADER_H_
