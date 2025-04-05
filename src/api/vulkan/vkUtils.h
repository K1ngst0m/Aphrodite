#pragma once

#include "common/arrayProxy.h"
#include "common/common.h"
#include "common/logger.h"
#include <vulkan/vulkan.hpp>

#include "api/gpuResource.h"

#include <source_location>

namespace aph::vk::utils
{
std::string errorString(::vk::Result errorCode);
std::string errorString(VkResult errorCode);
::vk::ImageAspectFlags getImageAspect(Format format);
::vk::SampleCountFlagBits getSampleCountFlags(uint32_t numSamples);
::vk::AccessFlags getAccessFlags(ResourceStateFlags state);
::vk::ImageLayout getImageLayout(ResourceStateFlags state);
Format getFormatFromVk(VkFormat format);
Result getResult(VkResult result);
Result getResult(::vk::Result result);
ImageUsageFlags getImageUsage(::vk::ImageUsageFlags usageFlags, ::vk::ImageCreateFlags createFlags = {});
PushConstantRange getPushConstantRange(const ::vk::PushConstantRange& vkRange);
ShaderStageFlags getShaderStages(::vk::ShaderStageFlags vkStages);
std::tuple<ResourceState, ::vk::AccessFlagBits2> getResourceState(BufferUsage usage, bool isWrite);
std::tuple<ResourceState, ::vk::AccessFlagBits2> getResourceState(ImageUsage usage, bool isWrite);
uint32_t getFormatSize(Format format);
} // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
std::tuple<::vk::ImageUsageFlags, ::vk::ImageCreateFlags> VkCast(ImageUsageFlags usage);
::vk::StencilOp VkCast(StencilOp op);
::vk::BlendOp VkCast(BlendOp op);
::vk::BlendFactor VkCast(BlendFactor factor);
::vk::CullModeFlags VkCast(CullMode mode);
::vk::FrontFace VkCast(WindingMode mode);
::vk::PolygonMode VkCast(PolygonMode mode);
::vk::PrimitiveTopology VkCast(PrimitiveTopology topology);
::vk::ShaderStageFlags VkCast(ShaderStageFlags stage);
::vk::ShaderStageFlagBits VkCast(ShaderStage stage);
::vk::ShaderStageFlags VkCast(ArrayProxy<ShaderStage> stages);
::vk::DebugUtilsLabelEXT VkCast(const DebugLabel& label);
::vk::Format VkCast(Format format);
::vk::IndexType VkCast(IndexType indexType);
::vk::CompareOp VkCast(CompareOp compareOp);
::vk::PipelineBindPoint VkCast(PipelineType type);
::vk::Filter VkCast(Filter filter);
::vk::SamplerAddressMode VkCast(SamplerAddressMode mode);
::vk::SamplerMipmapMode VkCast(SamplerMipmapMode mode);
::vk::ImageType VkCast(ImageType type);
::vk::ImageViewType VkCast(ImageViewType viewType);
::vk::BufferUsageFlags VkCast(BufferUsageFlags usage);
::vk::ImageLayout VkCast(ImageLayout layout);
::vk::AttachmentLoadOp VkCast(AttachmentLoadOp loadOp);
::vk::AttachmentStoreOp VkCast(AttachmentStoreOp storeOp);
::vk::ClearValue VkCast(const ClearValue& clearValue);
::vk::Rect2D VkCast(const Rect2D& rect);
::vk::Offset3D VkCast(const Offset3D& offset);
::vk::ImageSubresourceLayers VkCast(const ImageSubresourceLayers& subresourceLayers);
::vk::BufferImageCopy VkCast(const BufferImageCopy& bufferImageCopy);
::vk::PipelineStageFlagBits VkCast(PipelineStage stage);
::vk::PushConstantRange VkCast(const PushConstantRange& aphRange);
} // namespace aph::vk::utils

namespace aph
{
constexpr unsigned VULKAN_NUM_DESCRIPTOR_SETS = 4;
constexpr unsigned VULKAN_NUM_BINDINGS = 32;
constexpr unsigned VULKAN_NUM_BINDINGS_BINDLESS_VARYING = 16 * 1024;
constexpr unsigned VULKAN_NUM_ATTACHMENTS = 8;
constexpr unsigned VULKAN_NUM_VERTEX_ATTRIBS = 16;
constexpr unsigned VULKAN_NUM_VERTEX_BUFFERS = 4;
constexpr unsigned VULKAN_PUSH_CONSTANT_SIZE = 128;
constexpr unsigned VULKAN_MAX_UBO_SIZE = 16 * 1024;
constexpr unsigned VULKAN_NUM_USER_SPEC_CONSTANTS = 8;
constexpr unsigned VULKAN_NUM_RENDER_TARGETS = 32;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;
} // namespace aph

namespace aph::vk
{
const VkAllocationCallbacks* vkAllocator();
const ::vk::AllocationCallbacks& vk_allocator();
} // namespace aph::vk

namespace aph::vk::utils
{
constexpr std::string_view toString(QueueType type) noexcept
{
    switch (type)
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
    case ShaderStage::NA:
        return "NA";
    case ShaderStage::VS:
        return "VS";
    case ShaderStage::TCS:
        return "TCS";
    case ShaderStage::TES:
        return "TES";
    case ShaderStage::GS:
        return "GS";
    case ShaderStage::FS:
        return "FS";
    case ShaderStage::CS:
        return "CS";
    case ShaderStage::TS:
        return "TS";
    case ShaderStage::MS:
        return "MS";
    default:
        return "Unknown";
    }
}

inline ShaderStage getStageFromPath(std::string_view path)
{
    auto ext = std::filesystem::path(path).extension();
    if (ext == ".vert")
        return ShaderStage::VS;
    if (ext == ".tesc")
        return ShaderStage::TCS;
    if (ext == ".tese")
        return ShaderStage::TES;
    if (ext == ".geom")
        return ShaderStage::GS;
    if (ext == ".frag")
        return ShaderStage::FS;
    if (ext == ".comp")
        return ShaderStage::CS;
    return ShaderStage::NA;
}

} // namespace aph::vk::utils

namespace aph::vk
{
#ifdef APH_DEBUG
template <typename T>
inline void VK_VR(T result, const std::source_location source = std::source_location::current())
{
    if constexpr (std::is_same_v<VkResult, T>)
    {
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERR("Fatal : VkResult is \"%s\" in function[%s], %s:%d", utils::errorString(result).c_str(),
                       source.function_name(), source.file_name(), source.line());
            std::abort();
        }
    }
    else if constexpr (std::is_same_v<::vk::Result, T>)
    {
        if (result != ::vk::Result::eSuccess)
        {
            VK_LOG_ERR("Fatal : VkResult is \"%s\" in function[%s], %s:%d",
                       utils::errorString(static_cast<VkResult>(result)).c_str(), source.function_name(),
                       source.file_name(), source.line());
            std::abort();
        }
    }
    else
    {
        static_assert(dependent_false_v<T>, "Not a valid vulkan result.");
    }
}
#else
template <typename T>
inline void VK_VR(T result)
{
    return;
}
#endif
} // namespace aph::vk

