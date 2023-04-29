#ifndef VULKAN_SHADER_H_
#define VULKAN_SHADER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

class ShaderModule : public ResourceHandle<VkShaderModule>
{
public:
    static std::unique_ptr<ShaderModule> Create(Device* pDevice, const std::vector<char>& code,
                                                const std::string& entrypoint = "main");

    std::vector<char> getCode() { return m_code; }

private:
    ShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint = "main");
    std::string       m_entrypoint = {};
    std::vector<char> m_code       = {};
};

using ShaderMapList = std::unordered_map<ShaderStage, ShaderModule*>;

}  // namespace aph::vk

#endif  // SHADER_H_
