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
    static std::unique_ptr<ShaderModule> Create(Device* pDevice, const std::filesystem::path& path,
                                                   const std::string& entrypoint = "main");

    std::vector<uint32_t> getCode() { return m_code; }

private:
    ShaderModule(std::vector<uint32_t> code, VkShaderModule shaderModule, std::string entrypoint = "main");
    std::string       m_entrypoint = {};
    std::vector<uint32_t> m_code       = {};
};

using ShaderMapList = std::unordered_map<ShaderStage, ShaderModule*>;

}  // namespace aph::vk

#endif  // SHADER_H_
