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
    static VulkanShaderModule* Create(VulkanDevice* pDevice, const std::vector<char>& code,
                                      const std::string& entrypoint = "main");

    std::vector<char> getCode() { return m_code; }

private:
    VulkanShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint = "main");
    std::string       m_entrypoint = {};
    std::vector<char> m_code       = {};
};

using ShaderMapList = std::unordered_map<ShaderStage, VulkanShaderModule*>;

}  // namespace aph

#endif  // SHADER_H_
