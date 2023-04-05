#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;

class VulkanShaderModule : public ResourceHandle<VkShaderModule>
{
public:
    VulkanShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint = "main");

    std::vector<char> getCode() { return m_code; }

private:
    std::string m_entrypoint;
    std::vector<char> m_code;
};

class VulkanShaderCache
{
public:
    VulkanShaderCache(VulkanDevice *device) : m_device(device) {}
    VulkanShaderModule *getShaders(const std::string &path);
    void destroy();

private:
    VulkanDevice *m_device;
    std::unordered_map<std::string, VulkanShaderModule *> shaderModuleCaches;
};

using ShaderMapList = std::unordered_map<VkShaderStageFlagBits, VulkanShaderModule *>;

}  // namespace aph

#endif  // SHADER_H_
