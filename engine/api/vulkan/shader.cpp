#include "shader.h"
#include "device.h"

namespace aph
{
static VkShaderModule createShaderModule(VulkanDevice* device, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule shaderModule;

    VK_CHECK_RESULT(vkCreateShaderModule(device->getHandle(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

void VulkanShaderCache::destroy()
{
    for(auto& [key, shaderModule] : shaderModuleCaches)
    {
        vkDestroyShaderModule(m_device->getHandle(), shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }
}

VulkanShaderModule* VulkanShaderCache::getShaders(const std::filesystem::path& path)
{
    if(!shaderModuleCaches.count(path))
    {
        std::vector<char> spvCode;
        if(path.extension() == ".spv")
        {
            spvCode = aph::utils::loadSpvFromFile(path);
        }
        else
        {
            spvCode = aph::utils::loadGlslFromFile(path);
        }
        VkShaderModule shaderModule = createShaderModule(m_device, spvCode);

        shaderModuleCaches[path] = new VulkanShaderModule(spvCode, shaderModule);
    }
    return shaderModuleCaches[path];
}

VulkanShaderModule::VulkanShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint) :
    m_entrypoint(std::move(entrypoint)),
    m_code(std::move(code))
{
    getHandle() = shaderModule;
}
VulkanShaderCache::VulkanShaderCache(VulkanDevice* device) : m_device(device)
{
}
}  // namespace aph
