#include "vkUtils.h"
#include "spvgen/spvgen.h"

namespace aph::utils
{
VkShaderStageFlags    VkCast(const std::vector<ShaderStage>& stages)
{
    VkShaderStageFlags flags{};
    for(const auto& stage : stages)
    {
        flags |= VkCast(stage);
    }
    return flags;
}

VkDescriptorType VkCast(ResourceType type)
{
    switch(type)
    {
    case ResourceType::UNDEFINED:
    case ResourceType::SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
    case ResourceType::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ResourceType::COMBINE_SAMPLER_IMAGE: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case ResourceType::STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case ResourceType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case ResourceType::STORAGE_BUFFER: break; return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
}

VkShaderStageFlagBits VkCast(ShaderStage stage)
{
    switch(stage)
    {
    case ShaderStage::VS: return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::TCS: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TES: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderStage::GS: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::FS: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::CS: return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::TS: return VK_SHADER_STAGE_TASK_BIT_EXT;
    case ShaderStage::MS: return VK_SHADER_STAGE_MESH_BIT_EXT;
    default: return VK_SHADER_STAGE_ALL;
    }
}

std::vector<char> loadGlslFromFile(const std::string& filename)
{
    // TODO spvgen
    return {};
}

std::string errorString(VkResult errorCode)
{
    switch(errorCode)
    {
#define STR(r) \
    case VK_##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default: return "UNKNOWN_ERROR";
    }
}

std::vector<char> loadSpvFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    assert(file.is_open() && "failed to open file!");

    size_t            fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkImageAspectFlags getImageAspectFlags(VkFormat format)
{
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    default: return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VkImageLayout getDefaultImageLayoutFromUsage(VkImageUsageFlags usage)
{
    if(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if(usage & VK_IMAGE_USAGE_STORAGE_BIT) return VK_IMAGE_LAYOUT_GENERAL;
    if(usage & VK_IMAGE_USAGE_SAMPLED_BIT) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if(usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if(usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    if(usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    return VK_IMAGE_LAYOUT_GENERAL;
}

}  // namespace aph::utils
