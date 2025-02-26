#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vulkan/vulkan.hpp>
#include "common/common.h"
#include "common/logger.h"

#include "../gpuResource.h"

namespace aph::vk
{
#ifdef APH_DEBUG
    #define _VR(f) \
        { \
            VkResult res = (f); \
            if(res != VK_SUCCESS) \
            { \
                VK_LOG_ERR("Fatal : VkResult is \"%s\" in %s at line %d", aph::vk::utils::errorString(res).c_str(), __FILE__, \
                           __LINE__); \
                std::abort(); \
            } \
        }
#else
    #define _VR(f) (f);
#endif
}  // namespace aph::vk

namespace aph::vk::utils
{
std::string           errorString(VkResult errorCode);
VkImageAspectFlags    getImageAspect(VkFormat format);
VkImageAspectFlags    getImageAspect(Format format);
VkSampleCountFlagBits getSampleCountFlags(uint32_t numSamples);
VkAccessFlags         getAccessFlags(ResourceState state);
VkImageLayout         getImageLayout(ResourceState state);
Format                getFormatFromVk(VkFormat format);
Result                getResult(VkResult result);
VkResult              setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, std::string_view name);
}  // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
VkStencilOp           VkCast(StencilOp op);
VkBlendOp             VkCast(BlendOp op);
VkBlendFactor         VkCast(BlendFactor factor);
VkCullModeFlags       VkCast(CullMode mode);
VkFrontFace           VkCast(WindingMode mode);
VkPolygonMode         VkCast(PolygonMode mode);
VkPrimitiveTopology   VkCast(PrimitiveTopology topology);
VkShaderStageFlagBits VkCast(ShaderStage stage);
VkShaderStageFlags    VkCast(const std::vector<ShaderStage>& stages);
VkDebugUtilsLabelEXT  VkCast(const DebugLabel& label);
VkFormat              VkCast(Format format);
VkIndexType           VkCast(IndexType indexType);
VkCompareOp           VkCast(CompareOp compareOp);
VkPipelineBindPoint   VkCast(PipelineType type);
}  // namespace aph::vk::utils

namespace aph
{
constexpr unsigned VULKAN_NUM_DESCRIPTOR_SETS           = 4;
constexpr unsigned VULKAN_NUM_BINDINGS                  = 32;
constexpr unsigned VULKAN_NUM_BINDINGS_BINDLESS_VARYING = 16 * 1024;
constexpr unsigned VULKAN_NUM_ATTACHMENTS               = 8;
constexpr unsigned VULKAN_NUM_VERTEX_ATTRIBS            = 16;
constexpr unsigned VULKAN_NUM_VERTEX_BUFFERS            = 4;
constexpr unsigned VULKAN_PUSH_CONSTANT_SIZE            = 128;
constexpr unsigned VULKAN_MAX_UBO_SIZE                  = 16 * 1024;
constexpr unsigned VULKAN_NUM_USER_SPEC_CONSTANTS       = 8;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS   = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL    = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;
}  // namespace aph

namespace aph::vk
{
const VkAllocationCallbacks* vkAllocator();
const ::vk::AllocationCallbacks& vk_allocator();
}  // namespace aph::vk

namespace aph::vk::utils
{
constexpr std::string_view toString(QueueType type) noexcept
{
    switch(type)
    {
    case QueueType::Unsupport:
        return "Unsupport";
    case QueueType::Graphics:
        return "Graphics";
    case QueueType::Compute:
        return "Compute";
    case QueueType::Transfer:
        return "Transfer";
    case QueueType::Count:
        return "Count";
    default:
        return "Unknown";
    }
}

constexpr std::string toString(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::NA:  return "NA";
        case ShaderStage::VS:  return "VS";
        case ShaderStage::TCS: return "TCS";
        case ShaderStage::TES: return "TES";
        case ShaderStage::GS:  return "GS";
        case ShaderStage::FS:  return "FS";
        case ShaderStage::CS:  return "CS";
        case ShaderStage::TS:  return "TS";
        case ShaderStage::MS:  return "MS";
        default:               return "Unknown";
    }
}

inline ShaderStage getStageFromPath(std::string_view path)
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
}  // namespace aph::vk
