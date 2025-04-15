#pragma once

#include "common/arrayProxy.h"
#include "common/common.h"
#include "common/logger.h"
#include <vulkan/vulkan.hpp>

#include "api/gpuResource.h"

#include <source_location>

namespace aph::vk::utils
{
auto errorString(::vk::Result errorCode) -> std::string;
auto errorString(VkResult errorCode) -> std::string;
auto getImageAspect(Format format) -> ::vk::ImageAspectFlags;
auto getSampleCountFlags(uint32_t numSamples) -> ::vk::SampleCountFlagBits;
auto getAccessFlags(ResourceStateFlags state) -> ::vk::AccessFlags;
auto getImageLayout(ResourceStateFlags state) -> ::vk::ImageLayout;
auto getFormatFromVk(VkFormat format) -> Format;
auto getResult(VkResult result) -> Result;
auto getResult(::vk::Result result) -> Result;
auto getImageUsage(::vk::ImageUsageFlags usageFlags, ::vk::ImageCreateFlags createFlags = {}) -> ImageUsageFlags;
auto getPushConstantRange(const ::vk::PushConstantRange& vkRange) -> PushConstantRange;
auto getShaderStages(::vk::ShaderStageFlags vkStages) -> ShaderStageFlags;
auto getQueryType(::vk::QueryType vkQueryType) -> QueryType;
auto getPipelineStatistics(::vk::QueryPipelineStatisticFlags vkFlags) -> PipelineStatisticsFlags;
auto getResourceState(BufferUsage usage, bool isWrite) -> std::tuple<ResourceState, ::vk::AccessFlagBits2>;
auto getResourceState(ImageUsage usage, bool isWrite) -> std::tuple<ResourceState, ::vk::AccessFlagBits2>;
auto getFormatSize(Format format) -> uint32_t;
} // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
auto VkCast(ImageUsageFlags usage) -> std::tuple<::vk::ImageUsageFlags, ::vk::ImageCreateFlags>;
auto VkCast(StencilOp op) -> ::vk::StencilOp;
auto VkCast(BlendOp op) -> ::vk::BlendOp;
auto VkCast(BlendFactor factor) -> ::vk::BlendFactor;
auto VkCast(CullMode mode) -> ::vk::CullModeFlags;
auto VkCast(WindingMode mode) -> ::vk::FrontFace;
auto VkCast(PolygonMode mode) -> ::vk::PolygonMode;
auto VkCast(PrimitiveTopology topology) -> ::vk::PrimitiveTopology;
auto VkCast(ShaderStageFlags stage) -> ::vk::ShaderStageFlags;
auto VkCast(ShaderStage stage) -> ::vk::ShaderStageFlagBits;
auto VkCast(ArrayProxy<ShaderStage> stages) -> ::vk::ShaderStageFlags;
auto VkCast(const DebugLabel& label) -> ::vk::DebugUtilsLabelEXT;
auto VkCast(Format format) -> ::vk::Format;
auto VkCast(IndexType indexType) -> ::vk::IndexType;
auto VkCast(CompareOp compareOp) -> ::vk::CompareOp;
auto VkCast(PipelineType type) -> ::vk::PipelineBindPoint;
auto VkCast(Filter filter) -> ::vk::Filter;
auto VkCast(SamplerAddressMode mode) -> ::vk::SamplerAddressMode;
auto VkCast(SamplerMipmapMode mode) -> ::vk::SamplerMipmapMode;
auto VkCast(ImageType type) -> ::vk::ImageType;
auto VkCast(ImageViewType viewType) -> ::vk::ImageViewType;
auto VkCast(BufferUsageFlags usage) -> ::vk::BufferUsageFlags;
auto VkCast(ImageLayout layout) -> ::vk::ImageLayout;
auto VkCast(AttachmentLoadOp loadOp) -> ::vk::AttachmentLoadOp;
auto VkCast(AttachmentStoreOp storeOp) -> ::vk::AttachmentStoreOp;
auto VkCast(const ClearValue& clearValue) -> ::vk::ClearValue;
auto VkCast(const Rect2D& rect) -> ::vk::Rect2D;
auto VkCast(const Offset3D& offset) -> ::vk::Offset3D;
auto VkCast(const ImageSubresourceLayers& subresourceLayers) -> ::vk::ImageSubresourceLayers;
auto VkCast(const BufferImageCopy& bufferImageCopy) -> ::vk::BufferImageCopy;
auto VkCast(PipelineStage stage) -> ::vk::PipelineStageFlagBits;
auto VkCast(const PushConstantRange& aphRange) -> ::vk::PushConstantRange;
auto VkCast(QueryType queryType) -> ::vk::QueryType;
auto VkCast(PipelineStatisticsFlags flags) -> ::vk::QueryPipelineStatisticFlags;
} // namespace aph::vk::utils

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
constexpr unsigned VULKAN_NUM_RENDER_TARGETS            = 32;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS   = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL    = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;
} // namespace aph

namespace aph::vk
{
auto vkAllocator() -> const VkAllocationCallbacks*;
auto vk_allocator() -> const ::vk::AllocationCallbacks&;
} // namespace aph::vk

namespace aph::vk::utils
{
constexpr auto toString(QueueType type) noexcept -> std::string_view
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

constexpr auto toString(ShaderStage stage) -> std::string
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

inline auto getStageFromPath(std::string_view path) -> ShaderStage
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
inline auto VK_VR(T result, const std::source_location source = std::source_location::current()) -> void
{
    if constexpr (std::is_same_v<VkResult, T>)
    {
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERR("Fatal : VkResult is \"%s\" in function[%s], %s:%d",
                       utils::errorString(result).c_str(), source.function_name(), source.file_name(), source.line());
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
inline auto VK_VR(T result) -> void
{
    return;
}
#endif
} // namespace aph::vk
