#include "vkUtils.h"
#include <shaderc/shaderc.hpp>

namespace aph::vk::utils
{

std::string errorString(VkResult errorCode)
{
    switch(errorCode)
    {
#define STR(r) \
    case VK_##r: \
        return #r
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
    default:
        return "UNKNOWN_ERROR";
    }
}

VkShaderStageFlags VkCast(const std::vector<ShaderStage>& stages)
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
    case ResourceType::SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case ResourceType::SAMPLED_IMAGE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ResourceType::COMBINE_SAMPLER_IMAGE:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case ResourceType::STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case ResourceType::UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case ResourceType::STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
        assert("Invalid resource type.");
        return {};
    }
}

VkShaderStageFlagBits VkCast(ShaderStage stage)
{
    switch(stage)
    {
    case ShaderStage::VS:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::TCS:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TES:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderStage::GS:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::FS:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::CS:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::TS:
        return VK_SHADER_STAGE_TASK_BIT_EXT;
    case ShaderStage::MS:
        return VK_SHADER_STAGE_MESH_BIT_EXT;
    default:
        return VK_SHADER_STAGE_ALL;
    }
}

std::vector<uint32_t> loadGlslFromFile(const std::string& filename)
{
    shaderc::Compiler compiler{};
    std::string       source;
    auto              success = aph::utils::readFile(filename, source);
    APH_ASSERT(success);
    shaderc_shader_kind stage = shaderc_glsl_infer_from_source;
    switch(getStageFromPath(filename))
    {
    case ShaderStage::VS:
        stage = shaderc_vertex_shader;
        break;
    case ShaderStage::FS:
        stage = shaderc_fragment_shader;
        break;
    case ShaderStage::CS:
        stage = shaderc_compute_shader;
        break;
    case ShaderStage::TCS:
        stage = shaderc_tess_control_shader;
        break;
    case ShaderStage::TES:
        stage = shaderc_tess_evaluation_shader;
        break;
    case ShaderStage::GS:
        stage = shaderc_geometry_shader;
        break;
    case ShaderStage::TS:
        stage = shaderc_task_shader;
        break;
    case ShaderStage::MS:
        stage = shaderc_mesh_shader;
        break;
    default:
        break;
    }

    shaderc::CompileOptions options{};
    options.SetGenerateDebugInfo();
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);

    auto result = compiler.CompileGlslToSpv(source.data(), source.size(), stage, filename.c_str(), "main", options);
    APH_ASSERT(result.GetCompilationStatus() == 0);
    std::vector<uint32_t> spirv{result.cbegin(), result.cend()};
    return spirv;
}

std::vector<uint32_t> loadSpvFromFile(const std::string& filename)
{
    std::string source;
    auto        success = aph::utils::readFile(filename, source);
    APH_ASSERT(success);
    uint32_t              size = source.size();
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    memcpy(spirv.data(), source.data(), size);
    return spirv;
}

VkImageAspectFlags getImageAspect(VkFormat format)
{
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

ShaderStage getStageFromPath(std::string_view path)
{
    auto ext = std::filesystem::path(path).extension();
    if(ext == ".vert")
        return ShaderStage::VS;
    if(ext == ".tesc")
        return ShaderStage::TCS;
    if(ext == ".tese")
        return ShaderStage::TES;
    if(ext == ".geom")
        return ShaderStage::GS;
    if(ext == ".frag")
        return ShaderStage::FS;
    if(ext == ".comp")
        return ShaderStage::CS;
    return ShaderStage::NA;
}
VkSampleCountFlagBits getSampleCountFlags(uint32_t numSamples)
{
    if(numSamples <= 1)
    {
        return VK_SAMPLE_COUNT_1_BIT;
    }
    if(numSamples <= 2)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    if(numSamples <= 4)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if(numSamples <= 8)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if(numSamples <= 16)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if(numSamples <= 32)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    return VK_SAMPLE_COUNT_64_BIT;
}

VkDebugUtilsLabelEXT VkCast(const DebugLabel& label)
{
    VkDebugUtilsLabelEXT vkLabel{
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext      = nullptr,
        .pLabelName = label.name.c_str(),
        .color      = {label.color[0], label.color[1], label.color[2], label.color[3]},
    };

    return vkLabel;
}

}  // namespace aph::vk::utils

namespace aph::vk
{

#include <cstdlib>

#ifndef _MSC_VER
    #include <cstddef>
    #define MAX_ALIGN alignof(max_align_t)
#else
/* long double might be 128-bit, but our callers do not need that anyway(?) */
    #include <cstdint>
    #define MAX_ALIGN alignof(uint64_t)
#endif


static VKAPI_ATTR void* VKAPI_CALL vkDefaultAlloc(void* pUserData, size_t size, size_t alignment,
                                                  VkSystemAllocationScope allocationScope)
{
    APH_ASSERT(MAX_ALIGN % alignment == 0);
    return malloc(size);
}

static VKAPI_ATTR void* VKAPI_CALL vkDefaultRealloc(void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                                    VkSystemAllocationScope allocationScope)
{
    APH_ASSERT(MAX_ALIGN % alignment == 0);
    return realloc(pOriginal, size);
}

static VKAPI_ATTR void VKAPI_CALL vkDefaultFree(void* pUserData, void* pMemory)
{
    free(pMemory);
}

// #include "mmgr.h"
static VKAPI_ATTR void* VKAPI_CALL vkMMgrAlloc(void* pUserData, size_t size, size_t alignment,
                                               VkSystemAllocationScope allocationScope)
{
    VK_LOG_DEBUG("vulkan object alloc, size: %z, align: %z", size, alignment);
    APH_ASSERT(MAX_ALIGN % alignment == 0);
    return malloc(size);
}

static VKAPI_ATTR void* VKAPI_CALL vkMMgrRealloc(void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                                 VkSystemAllocationScope allocationScope)
{
    VK_LOG_DEBUG("vulkan object realloc, size: %z, align: %z", size, alignment);
    APH_ASSERT(MAX_ALIGN % alignment == 0);
    return realloc(pOriginal, size);
}

static VKAPI_ATTR void VKAPI_CALL vkMMgrFree(void* pUserData, void* pMemory)
{
    VK_LOG_DEBUG("vulkan object free, addr: %z", (size_t)pMemory);
    free(pMemory);
}

#define MMGR_ALLOC
const VkAllocationCallbacks* vkAllocator()
{
    static const VkAllocationCallbacks allocator = {
    #ifdef MMGR_ALLOC
        .pfnAllocation   = vkMMgrAlloc,
        .pfnReallocation = vkMMgrRealloc,
        .pfnFree         = vkMMgrFree,
    #else
        .pfnAllocation   = vkDefaultAlloc,
        .pfnReallocation = vkDefaultRealloc,
        .pfnFree         = vkDefaultFree,
    #endif
    };
    return &allocator;
}
}  // namespace aph::vk
