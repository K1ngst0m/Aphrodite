#include "shader.h"
#include "device.h"

namespace aph::vk
{
std::unique_ptr<ShaderModule> ShaderModule::Create(Device* pDevice, const std::filesystem::path& path,
                                                   const std::string& entrypoint)
{
    std::vector<uint32_t> spvCode;
    if(path.extension() == ".spv")
    {
        spvCode = utils::loadSpvFromFile(path);
    }
    else if(utils::getStageFromPath(path.c_str()) != ShaderStage::NA)
    {
        spvCode = utils::loadGlslFromFile(path);
    }

    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spvCode.size() * sizeof(spvCode[0]),
        .pCode    = spvCode.data(),
    };

    VkShaderModule handle;
    VK_CHECK_RESULT(vkCreateShaderModule(pDevice->getHandle(), &createInfo, nullptr, &handle));

    auto instance = std::unique_ptr<ShaderModule>(new ShaderModule(std::move(spvCode), handle, entrypoint));
    return instance;
}

ShaderModule::ShaderModule(std::vector<uint32_t> code, VkShaderModule shaderModule, std::string entrypoint) :
    m_entrypoint(std::move(entrypoint)),
    m_code(std::move(code))
{
    getHandle() = shaderModule;
}
}  // namespace aph::vk
