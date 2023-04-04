#include "shader.h"
#include "device.h"

namespace vkl
{
static VkShaderModule createShaderModule(VulkanDevice *device, const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;

    VK_CHECK_RESULT(vkCreateShaderModule(device->getHandle(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

void VulkanShaderCache::destroy()
{
    for(auto &[key, shaderModule] : shaderModuleCaches)
    {
        vkDestroyShaderModule(_device->getHandle(), shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }
}
VulkanShaderModule *VulkanShaderCache::getShaders(const std::string &path)
{
    if(!shaderModuleCaches.count(path))
    {
        std::vector<char> spvCode = vkl::utils::loadSpvFromFile(path);
        VkShaderModule shaderModule = createShaderModule(_device, spvCode);

        shaderModuleCaches[path] = new VulkanShaderModule(spvCode, shaderModule);
    }
    return shaderModuleCaches[path];
}
VulkanShaderModule::VulkanShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint) :
    _entrypoint(std::move(entrypoint)),
    _code(std::move(code))
{
    getHandle() = shaderModule;
}
}  // namespace vkl
