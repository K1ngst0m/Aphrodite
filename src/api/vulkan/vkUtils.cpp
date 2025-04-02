#include "vkUtils.h"

#include "common/arrayProxy.h"
#include "common/hash.h"

#include "allocator/allocator.h"

namespace aph::vk::utils
{
struct FormatMapping
{
    Format rhiFormat;
    VkFormat vkFormat;
};

static const std::array<FormatMapping, size_t(Format::COUNT)> FormatMap = { {
    { Format::Undefined, VK_FORMAT_UNDEFINED },
    { Format::R8_UINT, VK_FORMAT_R8_UINT },
    { Format::R8_SINT, VK_FORMAT_R8_SINT },
    { Format::R8_UNORM, VK_FORMAT_R8_UNORM },
    { Format::R8_SNORM, VK_FORMAT_R8_SNORM },
    { Format::RG8_UINT, VK_FORMAT_R8G8_UINT },
    { Format::RG8_SINT, VK_FORMAT_R8G8_SINT },
    { Format::RG8_UNORM, VK_FORMAT_R8G8_UNORM },
    { Format::RG8_SNORM, VK_FORMAT_R8G8_SNORM },
    { Format::RGB8_UINT, VK_FORMAT_R8G8B8_UINT },
    { Format::RGB8_SINT, VK_FORMAT_R8G8B8_SINT },
    { Format::RGB8_UNORM, VK_FORMAT_R8G8B8_UNORM },
    { Format::RGB8_SNORM, VK_FORMAT_R8G8B8_SNORM },
    { Format::R16_UINT, VK_FORMAT_R16_UINT },
    { Format::R16_SINT, VK_FORMAT_R16_SINT },
    { Format::R16_UNORM, VK_FORMAT_R16_UNORM },
    { Format::R16_SNORM, VK_FORMAT_R16_SNORM },
    { Format::R16_FLOAT, VK_FORMAT_R16_SFLOAT },
    { Format::BGRA4_UNORM, VK_FORMAT_B4G4R4A4_UNORM_PACK16 },
    { Format::B5G6R5_UNORM, VK_FORMAT_B5G6R5_UNORM_PACK16 },
    { Format::B5G5R5A1_UNORM, VK_FORMAT_B5G5R5A1_UNORM_PACK16 },
    { Format::RGBA8_UINT, VK_FORMAT_R8G8B8A8_UINT },
    { Format::RGBA8_SINT, VK_FORMAT_R8G8B8A8_SINT },
    { Format::RGBA8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
    { Format::RGBA8_SNORM, VK_FORMAT_R8G8B8A8_SNORM },
    { Format::BGRA8_UNORM, VK_FORMAT_B8G8R8A8_UNORM },
    { Format::SRGBA8_UNORM, VK_FORMAT_R8G8B8A8_SRGB },
    { Format::SBGRA8_UNORM, VK_FORMAT_B8G8R8A8_SRGB },
    { Format::R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
    { Format::R11G11B10_FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32 },
    { Format::RG16_UINT, VK_FORMAT_R16G16_UINT },
    { Format::RG16_SINT, VK_FORMAT_R16G16_SINT },
    { Format::RG16_UNORM, VK_FORMAT_R16G16_UNORM },
    { Format::RG16_SNORM, VK_FORMAT_R16G16_SNORM },
    { Format::RG16_FLOAT, VK_FORMAT_R16G16_SFLOAT },
    { Format::RGB16_UINT, VK_FORMAT_R16G16B16_UINT },
    { Format::RGB16_SINT, VK_FORMAT_R16G16B16_SINT },
    { Format::RGB16_UNORM, VK_FORMAT_R16G16B16_UNORM },
    { Format::RGB16_SNORM, VK_FORMAT_R16G16B16_SNORM },
    { Format::RGB16_FLOAT, VK_FORMAT_R16G16B16_SFLOAT },
    { Format::R32_UINT, VK_FORMAT_R32_UINT },
    { Format::R32_SINT, VK_FORMAT_R32_SINT },
    { Format::R32_FLOAT, VK_FORMAT_R32_SFLOAT },
    { Format::RGBA16_UINT, VK_FORMAT_R16G16B16A16_UINT },
    { Format::RGBA16_SINT, VK_FORMAT_R16G16B16A16_SINT },
    { Format::RGBA16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },
    { Format::RGBA16_UNORM, VK_FORMAT_R16G16B16A16_UNORM },
    { Format::RGBA16_SNORM, VK_FORMAT_R16G16B16A16_SNORM },
    { Format::RG32_UINT, VK_FORMAT_R32G32_UINT },
    { Format::RG32_SINT, VK_FORMAT_R32G32_SINT },
    { Format::RG32_FLOAT, VK_FORMAT_R32G32_SFLOAT },
    { Format::RGB32_UINT, VK_FORMAT_R32G32B32_UINT },
    { Format::RGB32_SINT, VK_FORMAT_R32G32B32_SINT },
    { Format::RGB32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT },
    { Format::RGBA32_UINT, VK_FORMAT_R32G32B32A32_UINT },
    { Format::RGBA32_SINT, VK_FORMAT_R32G32B32A32_SINT },
    { Format::RGBA32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT },
    { Format::D16, VK_FORMAT_D16_UNORM },
    { Format::D24S8, VK_FORMAT_D24_UNORM_S8_UINT },
    { Format::X24G8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
    { Format::D32, VK_FORMAT_D32_SFLOAT },
    { Format::D32S8, VK_FORMAT_D32_SFLOAT_S8_UINT },
    { Format::X32G8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT },
    { Format::BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK },
    { Format::BC1_UNORM_SRGB, VK_FORMAT_BC1_RGBA_SRGB_BLOCK },
    { Format::BC2_UNORM, VK_FORMAT_BC2_UNORM_BLOCK },
    { Format::BC2_UNORM_SRGB, VK_FORMAT_BC2_SRGB_BLOCK },
    { Format::BC3_UNORM, VK_FORMAT_BC3_UNORM_BLOCK },
    { Format::BC3_UNORM_SRGB, VK_FORMAT_BC3_SRGB_BLOCK },
    { Format::BC4_UNORM, VK_FORMAT_BC4_UNORM_BLOCK },
    { Format::BC4_SNORM, VK_FORMAT_BC4_SNORM_BLOCK },
    { Format::BC5_UNORM, VK_FORMAT_BC5_UNORM_BLOCK },
    { Format::BC5_SNORM, VK_FORMAT_BC5_SNORM_BLOCK },
    { Format::BC6H_UFLOAT, VK_FORMAT_BC6H_UFLOAT_BLOCK },
    { Format::BC6H_SFLOAT, VK_FORMAT_BC6H_SFLOAT_BLOCK },
    { Format::BC7_UNORM, VK_FORMAT_BC7_UNORM_BLOCK },
    { Format::BC7_UNORM_SRGB, VK_FORMAT_BC7_SRGB_BLOCK },

} };
static const HashMap<VkFormat, Format> VkToAphFormatMap = {
    { VK_FORMAT_UNDEFINED, Format::Undefined },
    { VK_FORMAT_R8_UINT, Format::R8_UINT },
    { VK_FORMAT_R8_SINT, Format::R8_SINT },
    { VK_FORMAT_R8_UNORM, Format::R8_UNORM },
    { VK_FORMAT_R8_SNORM, Format::R8_SNORM },
    { VK_FORMAT_R8G8_UINT, Format::RG8_UINT },
    { VK_FORMAT_R8G8_SINT, Format::RG8_SINT },
    { VK_FORMAT_R8G8_UNORM, Format::RG8_UNORM },
    { VK_FORMAT_R8G8_SNORM, Format::RG8_SNORM },
    { VK_FORMAT_R8G8B8_UINT, Format::RGB8_UINT },
    { VK_FORMAT_R8G8B8_SINT, Format::RGB8_SINT },
    { VK_FORMAT_R8G8B8_UNORM, Format::RGB8_UNORM },
    { VK_FORMAT_R8G8B8_SNORM, Format::RGB8_SNORM },
    { VK_FORMAT_R16_UINT, Format::R16_UINT },
    { VK_FORMAT_R16_SINT, Format::R16_SINT },
    { VK_FORMAT_R16_UNORM, Format::R16_UNORM },
    { VK_FORMAT_R16_SNORM, Format::R16_SNORM },
    { VK_FORMAT_R16_SFLOAT, Format::R16_FLOAT },
    { VK_FORMAT_B4G4R4A4_UNORM_PACK16, Format::BGRA4_UNORM },
    { VK_FORMAT_B5G6R5_UNORM_PACK16, Format::B5G6R5_UNORM },
    { VK_FORMAT_B5G5R5A1_UNORM_PACK16, Format::B5G5R5A1_UNORM },
    { VK_FORMAT_R8G8B8A8_UINT, Format::RGBA8_UINT },
    { VK_FORMAT_R8G8B8A8_SINT, Format::RGBA8_SINT },
    { VK_FORMAT_R8G8B8A8_UNORM, Format::RGBA8_UNORM },
    { VK_FORMAT_R8G8B8A8_SNORM, Format::RGBA8_SNORM },
    { VK_FORMAT_B8G8R8A8_UNORM, Format::BGRA8_UNORM },
    { VK_FORMAT_R8G8B8A8_SRGB, Format::SRGBA8_UNORM },
    { VK_FORMAT_B8G8R8A8_SRGB, Format::SBGRA8_UNORM },
    { VK_FORMAT_A2B10G10R10_UNORM_PACK32, Format::R10G10B10A2_UNORM },
    { VK_FORMAT_B10G11R11_UFLOAT_PACK32, Format::R11G11B10_FLOAT },
    { VK_FORMAT_R16G16_UINT, Format::RG16_UINT },
    { VK_FORMAT_R16G16_SINT, Format::RG16_SINT },
    { VK_FORMAT_R16G16_UNORM, Format::RG16_UNORM },
    { VK_FORMAT_R16G16_SNORM, Format::RG16_SNORM },
    { VK_FORMAT_R16G16_SFLOAT, Format::RG16_FLOAT },
    { VK_FORMAT_R16G16B16_UINT, Format::RGB16_UINT },
    { VK_FORMAT_R16G16B16_SINT, Format::RGB16_SINT },
    { VK_FORMAT_R16G16B16_UNORM, Format::RGB16_UNORM },
    { VK_FORMAT_R16G16B16_SNORM, Format::RGB16_SNORM },
    { VK_FORMAT_R16G16B16_SFLOAT, Format::RGB16_FLOAT },
    { VK_FORMAT_R32_UINT, Format::R32_UINT },
    { VK_FORMAT_R32_SINT, Format::R32_SINT },
    { VK_FORMAT_R32_SFLOAT, Format::R32_FLOAT },
    { VK_FORMAT_R16G16B16A16_UINT, Format::RGBA16_UINT },
    { VK_FORMAT_R16G16B16A16_SINT, Format::RGBA16_SINT },
    { VK_FORMAT_R16G16B16A16_SFLOAT, Format::RGBA16_FLOAT },
    { VK_FORMAT_R16G16B16A16_UNORM, Format::RGBA16_UNORM },
    { VK_FORMAT_R16G16B16A16_SNORM, Format::RGBA16_SNORM },
    { VK_FORMAT_R32G32_UINT, Format::RG32_UINT },
    { VK_FORMAT_R32G32_SINT, Format::RG32_SINT },
    { VK_FORMAT_R32G32_SFLOAT, Format::RG32_FLOAT },
    { VK_FORMAT_R32G32B32_UINT, Format::RGB32_UINT },
    { VK_FORMAT_R32G32B32_SINT, Format::RGB32_SINT },
    { VK_FORMAT_R32G32B32_SFLOAT, Format::RGB32_FLOAT },
    { VK_FORMAT_R32G32B32A32_UINT, Format::RGBA32_UINT },
    { VK_FORMAT_R32G32B32A32_SINT, Format::RGBA32_SINT },
    { VK_FORMAT_R32G32B32A32_SFLOAT, Format::RGBA32_FLOAT },
    { VK_FORMAT_D16_UNORM, Format::D16 },
    { VK_FORMAT_D24_UNORM_S8_UINT, Format::D24S8 }, // Chosen over X24G8_UINT
    { VK_FORMAT_D32_SFLOAT, Format::D32 },
    { VK_FORMAT_D32_SFLOAT_S8_UINT, Format::D32S8 }, // Chosen over X32G8_UINT
    { VK_FORMAT_BC1_RGBA_UNORM_BLOCK, Format::BC1_UNORM },
    { VK_FORMAT_BC1_RGBA_SRGB_BLOCK, Format::BC1_UNORM_SRGB },
    { VK_FORMAT_BC2_UNORM_BLOCK, Format::BC2_UNORM },
    { VK_FORMAT_BC2_SRGB_BLOCK, Format::BC2_UNORM_SRGB },
    { VK_FORMAT_BC3_UNORM_BLOCK, Format::BC3_UNORM },
    { VK_FORMAT_BC3_SRGB_BLOCK, Format::BC3_UNORM_SRGB },
    { VK_FORMAT_BC4_UNORM_BLOCK, Format::BC4_UNORM },
    { VK_FORMAT_BC4_SNORM_BLOCK, Format::BC4_SNORM },
    { VK_FORMAT_BC5_UNORM_BLOCK, Format::BC5_UNORM },
    { VK_FORMAT_BC5_SNORM_BLOCK, Format::BC5_SNORM },
    { VK_FORMAT_BC6H_UFLOAT_BLOCK, Format::BC6H_UFLOAT },
    { VK_FORMAT_BC6H_SFLOAT_BLOCK, Format::BC6H_SFLOAT },
    { VK_FORMAT_BC7_UNORM_BLOCK, Format::BC7_UNORM },
    { VK_FORMAT_BC7_SRGB_BLOCK, Format::BC7_UNORM_SRGB },
};

std::string errorString(VkResult errorCode)
{
    switch (errorCode)
    {
#define STR(r)   \
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

std::string errorString(::vk::Result errorCode)
{
    return errorString(static_cast<VkResult>(errorCode));
}

static constexpr std::pair<ShaderStage, ::vk::ShaderStageFlagBits> stageMap[] = {
    {ShaderStage::VS, ::vk::ShaderStageFlagBits::eVertex},
    {ShaderStage::TCS, ::vk::ShaderStageFlagBits::eTessellationControl},
    {ShaderStage::TES, ::vk::ShaderStageFlagBits::eTessellationEvaluation},
    {ShaderStage::GS, ::vk::ShaderStageFlagBits::eGeometry},
    {ShaderStage::FS, ::vk::ShaderStageFlagBits::eFragment},
    {ShaderStage::CS, ::vk::ShaderStageFlagBits::eCompute},
    {ShaderStage::TS, ::vk::ShaderStageFlagBits::eTaskEXT},
    {ShaderStage::MS, ::vk::ShaderStageFlagBits::eMeshEXT},
    {ShaderStage::All, ::vk::ShaderStageFlagBits::eAll}
};

::vk::ShaderStageFlagBits VkCast(ShaderStage stage)
{
    for (const auto& [shaderStage, vkFlag] : stageMap)
    {
        if (stage == shaderStage)
        {
            return vkFlag;
        }
    }
    return ::vk::ShaderStageFlagBits::eAll;
}
::vk::ShaderStageFlags VkCast(ArrayProxy<ShaderStage> stages)
{
    ::vk::ShaderStageFlags flags{};
    for (const auto& stage : stages)
    {
        flags |= VkCast(stage);
    }
    return flags;
}

::vk::ShaderStageFlags VkCast(ShaderStageFlags stage)
{
    ::vk::ShaderStageFlags flags{};
    for (const auto& [shaderStage, vkFlag] : stageMap)
    {
        if (stage & shaderStage)
        {
            flags |= vkFlag;
        }
    }
    return flags;
}

::vk::ImageAspectFlags getImageAspect(Format format)
{
    switch (format)
    {
    case Format::D16:
    case Format::D32:
        return ::vk::ImageAspectFlagBits::eDepth;
    case Format::D24S8:
    case Format::D32S8:
        return ::vk::ImageAspectFlagBits::eDepth | ::vk::ImageAspectFlagBits::eStencil;
    default:
        return ::vk::ImageAspectFlagBits::eColor;
    }
}

::vk::SampleCountFlagBits getSampleCountFlags(uint32_t numSamples)
{
    if (numSamples <= 1)
    {
        return ::vk::SampleCountFlagBits::e1;
    }
    if (numSamples <= 2)
    {
        return ::vk::SampleCountFlagBits::e2;
    }
    if (numSamples <= 4)
    {
        return ::vk::SampleCountFlagBits::e4;
    }
    if (numSamples <= 8)
    {
        return ::vk::SampleCountFlagBits::e8;
    }
    if (numSamples <= 16)
    {
        return ::vk::SampleCountFlagBits::e16;
    }
    if (numSamples <= 32)
    {
        return ::vk::SampleCountFlagBits::e32;
    }
    return ::vk::SampleCountFlagBits::e64;
}

::vk::DebugUtilsLabelEXT VkCast(const DebugLabel& label)
{
    ::vk::DebugUtilsLabelEXT vkLabel{};
    vkLabel.setPLabelName(label.name.c_str()).setColor(label.color);
    return vkLabel;
}

::vk::AccessFlags getAccessFlags(ResourceStateFlags state)
{
    ::vk::AccessFlags flags; // ::vk::AccessFlags is a bitmask (typedef of VkAccessFlags)

    if ((state & ResourceState::CopySource))
    {
        flags |= ::vk::AccessFlagBits::eTransferRead;
    }
    if ((state & ResourceState::CopyDest))
    {
        flags |= ::vk::AccessFlagBits::eTransferWrite;
    }
    if ((state & ResourceState::VertexBuffer))
    {
        flags |= ::vk::AccessFlagBits::eVertexAttributeRead;
    }
    if ((state & ResourceState::UniformBuffer))
    {
        flags |= ::vk::AccessFlagBits::eUniformRead;
    }
    if ((state & ResourceState::IndexBuffer))
    {
        flags |= ::vk::AccessFlagBits::eIndexRead;
    }
    if ((state & ResourceState::UnorderedAccess))
    {
        flags |= ::vk::AccessFlagBits::eShaderRead | ::vk::AccessFlagBits::eShaderWrite;
    }
    if ((state & ResourceState::IndirectArgument))
    {
        flags |= ::vk::AccessFlagBits::eIndirectCommandRead;
    }
    if ((state & ResourceState::RenderTarget))
    {
        flags |= ::vk::AccessFlagBits::eColorAttachmentRead | ::vk::AccessFlagBits::eColorAttachmentWrite;
    }
    if ((state & ResourceState::DepthStencil))
    {
        flags |= ::vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    }
    if ((state & ResourceState::ShaderResource))
    {
        flags |= ::vk::AccessFlagBits::eShaderRead;
    }
    if ((state & ResourceState::Present))
    {
        flags |= ::vk::AccessFlagBits::eMemoryRead;
    }
    if ((state & ResourceState::AccelStructRead))
    {
        flags |= ::vk::AccessFlagBits::eAccelerationStructureReadKHR;
    }
    if ((state & ResourceState::AccelStructWrite))
    {
        flags |= ::vk::AccessFlagBits::eAccelerationStructureWriteKHR;
    }

    return flags;
}

::vk::ImageLayout getImageLayout(ResourceStateFlags state)
{
    if ((state & ResourceState::CopySource))
        return ::vk::ImageLayout::eTransferSrcOptimal;

    if ((state & ResourceState::CopyDest))
        return ::vk::ImageLayout::eTransferDstOptimal;

    if ((state & ResourceState::RenderTarget))
        return ::vk::ImageLayout::eColorAttachmentOptimal;

    if ((state & ResourceState::DepthStencil))
        return ::vk::ImageLayout::eDepthStencilAttachmentOptimal;

    if ((state & ResourceState::UnorderedAccess))
        return ::vk::ImageLayout::eGeneral;

    if ((state & ResourceState::ShaderResource))
        return ::vk::ImageLayout::eShaderReadOnlyOptimal;

    if ((state & ResourceState::Present))
        return ::vk::ImageLayout::ePresentSrcKHR;

    if ((state & ResourceState::General))
        return ::vk::ImageLayout::eGeneral;

    return ::vk::ImageLayout::eUndefined;
}

::vk::Format VkCast(Format format)
{
    APH_ASSERT(format < Format::COUNT);
    APH_ASSERT(FormatMap[uint32_t(format)].rhiFormat == format);

    return static_cast<::vk::Format>(FormatMap[uint32_t(format)].vkFormat);
}

Format getFormatFromVk(VkFormat format)
{
    APH_ASSERT(VkToAphFormatMap.contains(format));
    return VkToAphFormatMap.at(format);
}

Result getResult(::vk::Result result)
{
    switch (result)
    {
    case ::vk::Result::eSuccess:
        return Result::Success;
    default:
        return { Result::RuntimeError, errorString(result) };
    }
}

Result getResult(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return Result::Success;
    default:
        return { Result::RuntimeError, errorString(result) };
    }
}
::vk::IndexType VkCast(IndexType indexType)
{
    switch (indexType)
    {
    case IndexType::UINT16:
        return ::vk::IndexType::eUint16;
    case IndexType::UINT32:
        return ::vk::IndexType::eUint32;
    default:
        APH_ASSERT(false);
        return ::vk::IndexType::eNoneKHR;
    }
}

::vk::CompareOp VkCast(CompareOp compareOp)
{
    switch (compareOp)
    {
    case CompareOp::Never:
        return ::vk::CompareOp::eNever;
    case CompareOp::Less:
        return ::vk::CompareOp::eLess;
    case CompareOp::Equal:
        return ::vk::CompareOp::eEqual;
    case CompareOp::LessEqual:
        return ::vk::CompareOp::eLessOrEqual;
    case CompareOp::Greater:
        return ::vk::CompareOp::eGreater;
    case CompareOp::NotEqual:
        return ::vk::CompareOp::eNotEqual;
    case CompareOp::GreaterEqual:
        return ::vk::CompareOp::eGreaterOrEqual;
    case CompareOp::Always:
        return ::vk::CompareOp::eAlways;
    }

    APH_ASSERT(false);
    return ::vk::CompareOp::eAlways;
}

::vk::PrimitiveTopology VkCast(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::PointList:
        return ::vk::PrimitiveTopology::ePointList;
    case PrimitiveTopology::LineList:
        return ::vk::PrimitiveTopology::eLineList;
    case PrimitiveTopology::LineStrip:
        return ::vk::PrimitiveTopology::eLineStrip;
    case PrimitiveTopology::TriangleList:
        return ::vk::PrimitiveTopology::eTriangleList;
    case PrimitiveTopology::TriangleStrip:
        return ::vk::PrimitiveTopology::eTriangleStrip;
    case PrimitiveTopology::TriangleFan:
        return ::vk::PrimitiveTopology::eTriangleFan;
    case PrimitiveTopology::LineListWithAdjacency:
        return ::vk::PrimitiveTopology::eLineListWithAdjacency;
    case PrimitiveTopology::LineStripWithAdjacency:
        return ::vk::PrimitiveTopology::eLineStripWithAdjacency;
    case PrimitiveTopology::TriangleListWithAdjacency:
        return ::vk::PrimitiveTopology::eTriangleListWithAdjacency;
    case PrimitiveTopology::TriangleStripWithAdjacency:
        return ::vk::PrimitiveTopology::eTriangleStripWithAdjacency;
    case PrimitiveTopology::PatchList:
        return ::vk::PrimitiveTopology::ePatchList;
    default:
        APH_ASSERT(false);
        return ::vk::PrimitiveTopology::eTriangleList;
    }
}

::vk::CullModeFlags VkCast(CullMode mode)
{
    switch (mode)
    {
    case CullMode::None:
        return ::vk::CullModeFlagBits::eNone;
    case CullMode::Front:
        return ::vk::CullModeFlagBits::eFront;
    case CullMode::Back:
        return ::vk::CullModeFlagBits::eBack;
    }
    APH_ASSERT(false);
    return ::vk::CullModeFlagBits::eNone;
}

::vk::FrontFace VkCast(WindingMode mode)
{
    switch (mode)
    {
    case WindingMode::CCW:
        return ::vk::FrontFace::eCounterClockwise;
    case WindingMode::CW:
        return ::vk::FrontFace::eClockwise;
    }
    APH_ASSERT(false);
    return ::vk::FrontFace::eCounterClockwise;
}

::vk::PolygonMode VkCast(PolygonMode mode)
{
    switch (mode)
    {
    case PolygonMode::Fill:
        return ::vk::PolygonMode::eFill;
    case PolygonMode::Line:
        return ::vk::PolygonMode::eLine;
    }
    APH_ASSERT(false);
    return ::vk::PolygonMode::eFill;
}
::vk::BlendFactor VkCast(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:
        return ::vk::BlendFactor::eZero;
    case BlendFactor::One:
        return ::vk::BlendFactor::eOne;
    case BlendFactor::SrcColor:
        return ::vk::BlendFactor::eSrcColor;
    case BlendFactor::OneMinusSrcColor:
        return ::vk::BlendFactor::eOneMinusSrcColor;
    case BlendFactor::SrcAlpha:
        return ::vk::BlendFactor::eSrcAlpha;
    case BlendFactor::OneMinusSrcAlpha:
        return ::vk::BlendFactor::eOneMinusSrcAlpha;
    case BlendFactor::DstColor:
        return ::vk::BlendFactor::eDstColor;
    case BlendFactor::OneMinusDstColor:
        return ::vk::BlendFactor::eOneMinusDstColor;
    case BlendFactor::DstAlpha:
        return ::vk::BlendFactor::eDstAlpha;
    case BlendFactor::OneMinusDstAlpha:
        return ::vk::BlendFactor::eOneMinusDstAlpha;
    case BlendFactor::SrcAlphaSaturated:
        return ::vk::BlendFactor::eSrcAlphaSaturate;
    case BlendFactor::Src1Color:
        return ::vk::BlendFactor::eSrc1Color;
    case BlendFactor::OneMinusSrc1Color:
        return ::vk::BlendFactor::eOneMinusSrc1Color;
    case BlendFactor::Src1Alpha:
        return ::vk::BlendFactor::eSrc1Alpha;
    case BlendFactor::OneMinusSrc1Alpha:
        return ::vk::BlendFactor::eOneMinusSrc1Alpha;
    default:
        APH_ASSERT(false);
        return ::vk::BlendFactor::eZero;
        break;
    }

    APH_ASSERT(false);
    return ::vk::BlendFactor::eZero;
}

::vk::BlendOp VkCast(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Add:
        return ::vk::BlendOp::eAdd;
    case BlendOp::Subtract:
        return ::vk::BlendOp::eSubtract;
    case BlendOp::ReverseSubtract:
        return ::vk::BlendOp::eReverseSubtract;
    case BlendOp::Min:
        return ::vk::BlendOp::eMin;
    case BlendOp::Max:
        return ::vk::BlendOp::eMax;
    }
    APH_ASSERT(false);
    return ::vk::BlendOp::eAdd;
}

::vk::StencilOp VkCast(StencilOp op)
{
    switch (op)
    {
    case StencilOp::Keep:
        return ::vk::StencilOp::eKeep;
    case StencilOp::Zero:
        return ::vk::StencilOp::eZero;
    case StencilOp::Replace:
        return ::vk::StencilOp::eReplace;
    case StencilOp::IncrementClamp:
        return ::vk::StencilOp::eIncrementAndClamp;
    case StencilOp::DecrementClamp:
        return ::vk::StencilOp::eDecrementAndClamp;
    case StencilOp::Invert:
        return ::vk::StencilOp::eInvert;
    case StencilOp::IncrementWrap:
        return ::vk::StencilOp::eIncrementAndWrap;
    case StencilOp::DecrementWrap:
        return ::vk::StencilOp::eDecrementAndWrap;
    }
    APH_ASSERT(false);
    return ::vk::StencilOp::eKeep;
}

::vk::PipelineBindPoint VkCast(PipelineType type)
{
    switch (type)
    {
    case PipelineType::Geometry:
    case PipelineType::Mesh:
        return ::vk::PipelineBindPoint::eGraphics;
    case PipelineType::Compute:
        return ::vk::PipelineBindPoint::eCompute;
    case PipelineType::RayTracing:
        return ::vk::PipelineBindPoint::eRayTracingKHR;
    default:
        APH_ASSERT(false);
        return ::vk::PipelineBindPoint::eGraphics;
    }
}
::vk::Filter VkCast(Filter filter)
{
    switch (filter)
    {
    case Filter::Nearest:
        return ::vk::Filter::eNearest;
    case Filter::Linear:
        return ::vk::Filter::eLinear;
    case Filter::Cubic:
        return ::vk::Filter::eCubicEXT;
    }
    APH_ASSERT(false && "Unhandled Filter enum type");
    return ::vk::Filter::eNearest;
}
::vk::SamplerAddressMode VkCast(SamplerAddressMode mode)
{
    switch (mode)
    {
    case SamplerAddressMode::Repeat:
        return ::vk::SamplerAddressMode::eRepeat;
    case SamplerAddressMode::MirroredRepeat:
        return ::vk::SamplerAddressMode::eMirroredRepeat;
    case SamplerAddressMode::ClampToEdge:
        return ::vk::SamplerAddressMode::eClampToEdge;
    case SamplerAddressMode::ClampToBorder:
        return ::vk::SamplerAddressMode::eClampToBorder;
    case SamplerAddressMode::MirrorClampToEdge:
        return ::vk::SamplerAddressMode::eMirrorClampToEdge;
    }
    assert(false && "Unhandled SamplerAddressMode enum type");
    return ::vk::SamplerAddressMode::eRepeat;
}
::vk::SamplerMipmapMode VkCast(SamplerMipmapMode mode)
{
    switch (mode)
    {
    case SamplerMipmapMode::Nearest:
        return ::vk::SamplerMipmapMode::eNearest;
    case SamplerMipmapMode::Linear:
        return ::vk::SamplerMipmapMode::eLinear;
    }
    APH_ASSERT(false && "Unhandled SamplerMipmapMode enum type");
    return ::vk::SamplerMipmapMode::eNearest;
}
::vk::ImageViewType VkCast(ImageViewType viewType)
{
    switch (viewType)
    {
    case ImageViewType::e1D:
        return ::vk::ImageViewType::e1D;
    case ImageViewType::e2D:
        return ::vk::ImageViewType::e2D;
    case ImageViewType::e3D:
        return ::vk::ImageViewType::e3D;
    case ImageViewType::Cube:
        return ::vk::ImageViewType::eCube;
    }
    APH_ASSERT(false && "Unhandled ImageViewType in VkCast");
    return ::vk::ImageViewType::e1D; // Fallback
}
::vk::ImageType VkCast(ImageType type)
{
    switch (type)
    {
    case ImageType::e1D:
        return ::vk::ImageType::e1D;
    case ImageType::e2D:
        return ::vk::ImageType::e2D;
    case ImageType::e3D:
        return ::vk::ImageType::e3D;
    }
    APH_ASSERT(false && "Unhandled ImageType in VkCast");
    return ::vk::ImageType::e1D; // Fallback
}

::vk::BufferUsageFlags VkCast(BufferUsageFlags usage)
{
    ::vk::BufferUsageFlags vkUsage;

    if (usage & BufferUsage::Vertex)
        vkUsage |= ::vk::BufferUsageFlagBits::eVertexBuffer;
    if (usage & BufferUsage::Index)
        vkUsage |= ::vk::BufferUsageFlagBits::eIndexBuffer;
    if (usage & BufferUsage::Uniform)
        vkUsage |= ::vk::BufferUsageFlagBits::eUniformBuffer;
    if (usage & BufferUsage::Storage)
        vkUsage |= ::vk::BufferUsageFlagBits::eStorageBuffer;
    if (usage & BufferUsage::Indirect)
        vkUsage |= ::vk::BufferUsageFlagBits::eIndirectBuffer;
    if (usage & BufferUsage::TransferSrc)
        vkUsage |= ::vk::BufferUsageFlagBits::eTransferSrc;
    if (usage & BufferUsage::TransferDst)
        vkUsage |= ::vk::BufferUsageFlagBits::eTransferDst;
    if (usage & BufferUsage::AccelStructBuild)
        vkUsage |= ::vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
    if (usage & BufferUsage::AccelStructStorage)
        vkUsage |= ::vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
    if (usage & BufferUsage::ShaderBindingTable)
        vkUsage |= ::vk::BufferUsageFlagBits::eShaderBindingTableKHR;
    if (usage & BufferUsage::ShaderDeviceAddress)
        vkUsage |= ::vk::BufferUsageFlagBits::eShaderDeviceAddress;

    return vkUsage;
}

std::tuple<::vk::ImageUsageFlags, ::vk::ImageCreateFlags> VkCast(ImageUsageFlags usage)
{
    ::vk::ImageUsageFlags usageFlags = {};
    ::vk::ImageCreateFlags createFlags = {};

    // Map usage flags (lower 16 bits)
    if (usage & ImageUsage::TransferSrc)
        usageFlags |= ::vk::ImageUsageFlagBits::eTransferSrc;
    if (usage & ImageUsage::TransferDst)
        usageFlags |= ::vk::ImageUsageFlagBits::eTransferDst;
    if (usage & ImageUsage::Sampled)
        usageFlags |= ::vk::ImageUsageFlagBits::eSampled;
    if (usage & ImageUsage::Storage)
        usageFlags |= ::vk::ImageUsageFlagBits::eStorage;
    if (usage & ImageUsage::ColorAttachment)
        usageFlags |= ::vk::ImageUsageFlagBits::eColorAttachment;
    if (usage & ImageUsage::DepthStencil)
        usageFlags |= ::vk::ImageUsageFlagBits::eDepthStencilAttachment;
    if (usage & ImageUsage::Transient)
        usageFlags |= ::vk::ImageUsageFlagBits::eTransientAttachment;
    if (usage & ImageUsage::InputAttachment)
        usageFlags |= ::vk::ImageUsageFlagBits::eInputAttachment;

    // Map create flags (upper 16 bits)
    if (usage & ImageUsage::SparseBinding)
        createFlags |= ::vk::ImageCreateFlagBits::eSparseBinding;
    if (usage & ImageUsage::SparseResidency)
        createFlags |= ::vk::ImageCreateFlagBits::eSparseResidency;
    if (usage & ImageUsage::SparseAliased)
        createFlags |= ::vk::ImageCreateFlagBits::eSparseAliased;
    if (usage & ImageUsage::MutableFormat)
        createFlags |= ::vk::ImageCreateFlagBits::eMutableFormat;
    if (usage & ImageUsage::CubeCompatible)
        createFlags |= ::vk::ImageCreateFlagBits::eCubeCompatible;
    if (usage & ImageUsage::Array2DCompatible)
        createFlags |= ::vk::ImageCreateFlagBits::e2DArrayCompatible;
    if (usage & ImageUsage::BlockTexelView)
        createFlags |= ::vk::ImageCreateFlagBits::eBlockTexelViewCompatible;

    return { usageFlags, createFlags };
}

ImageUsageFlags getImageUsage(::vk::ImageUsageFlags usageFlags, ::vk::ImageCreateFlags createFlags)
{
    ImageUsageFlags result = ImageUsage::None;

    // Map usage flags (lower 16 bits)
    if (usageFlags & ::vk::ImageUsageFlagBits::eTransferSrc)
        result |= ImageUsage::TransferSrc;
    if (usageFlags & ::vk::ImageUsageFlagBits::eTransferDst)
        result |= ImageUsage::TransferDst;
    if (usageFlags & ::vk::ImageUsageFlagBits::eSampled)
        result |= ImageUsage::Sampled;
    if (usageFlags & ::vk::ImageUsageFlagBits::eStorage)
        result |= ImageUsage::Storage;
    if (usageFlags & ::vk::ImageUsageFlagBits::eColorAttachment)
        result |= ImageUsage::ColorAttachment;
    if (usageFlags & ::vk::ImageUsageFlagBits::eDepthStencilAttachment)
        result |= ImageUsage::DepthStencil;
    if (usageFlags & ::vk::ImageUsageFlagBits::eTransientAttachment)
        result |= ImageUsage::Transient;
    if (usageFlags & ::vk::ImageUsageFlagBits::eInputAttachment)
        result |= ImageUsage::InputAttachment;

    // Map create flags (upper 16 bits)
    if (createFlags & ::vk::ImageCreateFlagBits::eSparseBinding)
        result |= ImageUsage::SparseBinding;
    if (createFlags & ::vk::ImageCreateFlagBits::eSparseResidency)
        result |= ImageUsage::SparseResidency;
    if (createFlags & ::vk::ImageCreateFlagBits::eSparseAliased)
        result |= ImageUsage::SparseAliased;
    if (createFlags & ::vk::ImageCreateFlagBits::eMutableFormat)
        result |= ImageUsage::MutableFormat;
    if (createFlags & ::vk::ImageCreateFlagBits::eCubeCompatible)
        result |= ImageUsage::CubeCompatible;
    if (createFlags & ::vk::ImageCreateFlagBits::e2DArrayCompatible)
        result |= ImageUsage::Array2DCompatible;
    if (createFlags & ::vk::ImageCreateFlagBits::eBlockTexelViewCompatible)
        result |= ImageUsage::BlockTexelView;

    return result;
}

std::tuple<ResourceState, ::vk::AccessFlagBits2> getResourceState(BufferUsage usage, bool isWrite)
{
    ResourceState state = ResourceState::General;
    ::vk::AccessFlagBits2 accessFlags = ::vk::AccessFlagBits2::eShaderRead;

    if (usage & BufferUsage::Uniform)
    {
        state = ResourceState::UniformBuffer;
        accessFlags = ::vk::AccessFlagBits2::eShaderRead;
    }
    else if (usage & BufferUsage::Storage)
    {
        state = ResourceState::UnorderedAccess;
        accessFlags = isWrite ? ::vk::AccessFlagBits2::eShaderWrite : ::vk::AccessFlagBits2::eShaderStorageRead;
    }
    else if (usage & BufferUsage::Index)
    {
        state = ResourceState::IndexBuffer;
        accessFlags = ::vk::AccessFlagBits2::eIndexRead;
    }
    else if (usage & BufferUsage::Vertex)
    {
        state = ResourceState::VertexBuffer;
        accessFlags = ::vk::AccessFlagBits2::eVertexAttributeRead;
    }
    else if (usage & BufferUsage::TransferDst)
    {
        state = ResourceState::CopyDest;
        accessFlags = ::vk::AccessFlagBits2::eTransferWrite;
    }
    else if (usage & BufferUsage::TransferSrc)
    {
        state = ResourceState::CopySource;
        accessFlags = ::vk::AccessFlagBits2::eTransferRead;
    }
    else if (usage & BufferUsage::Indirect)
    {
        state = ResourceState::IndirectArgument;
        accessFlags = ::vk::AccessFlagBits2::eIndirectCommandRead;
    }
    else
    {
        VK_LOG_WARN("Unspecified buffer usage type, defaulting to general access");
        state = ResourceState::General;
        accessFlags = isWrite ? ::vk::AccessFlagBits2::eShaderWrite : ::vk::AccessFlagBits2::eShaderRead;
    }

    return { state, accessFlags };
}

std::tuple<ResourceState, ::vk::AccessFlagBits2> getResourceState(ImageUsage usage, bool isWrite)
{
    ResourceState state = ResourceState::General;
    ::vk::AccessFlagBits2 accessFlags =
        isWrite ? ::vk::AccessFlagBits2::eShaderWrite : ::vk::AccessFlagBits2::eShaderRead;

    if (usage & ImageUsage::Sampled)
    {
        state = ResourceState::ShaderResource;
        accessFlags = ::vk::AccessFlagBits2::eShaderSampledRead;
    }
    else if (usage & ImageUsage::Storage)
    {
        state = ResourceState::UnorderedAccess;
        accessFlags = isWrite ? ::vk::AccessFlagBits2::eShaderStorageWrite : ::vk::AccessFlagBits2::eShaderStorageRead;
    }
    else if (usage & ImageUsage::TransferSrc)
    {
        state = ResourceState::CopySource;
        accessFlags = ::vk::AccessFlagBits2::eTransferRead;
    }
    else if (usage & ImageUsage::TransferDst)
    {
        state = ResourceState::CopyDest;
        accessFlags = ::vk::AccessFlagBits2::eTransferWrite;
    }
    else if (usage & ImageUsage::ColorAttachment)
    {
        state = ResourceState::RenderTarget;
        accessFlags =
            isWrite ? ::vk::AccessFlagBits2::eColorAttachmentWrite : ::vk::AccessFlagBits2::eColorAttachmentRead;
    }
    else if (usage & ImageUsage::DepthStencil)
    {
        state = ResourceState::DepthStencil;
        accessFlags = isWrite ? ::vk::AccessFlagBits2::eDepthStencilAttachmentWrite :
                                ::vk::AccessFlagBits2::eDepthStencilAttachmentRead;
    }
    else
    {
        // Default for other usage flags
        VK_LOG_WARN("Unspecified image usage type, defaulting to general access");
        state = ResourceState::General;
        accessFlags = isWrite ? ::vk::AccessFlagBits2::eShaderWrite : ::vk::AccessFlagBits2::eShaderRead;
    }

    return { state, accessFlags };
}

::vk::ImageLayout VkCast(ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout::Undefined:
        return ::vk::ImageLayout::eUndefined;
    case ImageLayout::General:
        return ::vk::ImageLayout::eGeneral;
    case ImageLayout::ColorAttachmentOptimal:
        return ::vk::ImageLayout::eColorAttachmentOptimal;
    case ImageLayout::DepthStencilAttachmentOptimal:
        return ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ImageLayout::DepthStencilReadOnlyOptimal:
        return ::vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case ImageLayout::ShaderReadOnlyOptimal:
        return ::vk::ImageLayout::eShaderReadOnlyOptimal;
    case ImageLayout::TransferSrcOptimal:
        return ::vk::ImageLayout::eTransferSrcOptimal;
    case ImageLayout::TransferDstOptimal:
        return ::vk::ImageLayout::eTransferDstOptimal;
    case ImageLayout::Preinitialized:
        return ::vk::ImageLayout::ePreinitialized;
    case ImageLayout::PresentSrc:
        return ::vk::ImageLayout::ePresentSrcKHR;
    case ImageLayout::DepthReadOnlyStencilAttachmentOptimal:
        return ::vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal;
    case ImageLayout::DepthAttachmentStencilReadOnlyOptimal:
        return ::vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
    case ImageLayout::DepthAttachmentOptimal:
        return ::vk::ImageLayout::eDepthAttachmentOptimal;
    case ImageLayout::DepthReadOnlyOptimal:
        return ::vk::ImageLayout::eDepthReadOnlyOptimal;
    case ImageLayout::StencilAttachmentOptimal:
        return ::vk::ImageLayout::eStencilAttachmentOptimal;
    case ImageLayout::StencilReadOnlyOptimal:
        return ::vk::ImageLayout::eStencilReadOnlyOptimal;
    case ImageLayout::AttachmentOptimal:
        return ::vk::ImageLayout::eAttachmentOptimal;
    case ImageLayout::ReadOnlyOptimal:
        return ::vk::ImageLayout::eReadOnlyOptimal;
    default:
        APH_ASSERT(false && "Unhandled ImageLayout in VkCast");
        return ::vk::ImageLayout::eUndefined;
    }
}

::vk::AttachmentLoadOp VkCast(AttachmentLoadOp loadOp)
{
    switch (loadOp)
    {
    case AttachmentLoadOp::Load:
        return ::vk::AttachmentLoadOp::eLoad;
    case AttachmentLoadOp::Clear:
        return ::vk::AttachmentLoadOp::eClear;
    case AttachmentLoadOp::DontCare:
        return ::vk::AttachmentLoadOp::eDontCare;
    default:
        APH_ASSERT(false && "Unhandled AttachmentLoadOp in VkCast");
        return ::vk::AttachmentLoadOp::eDontCare;
    }
}

::vk::AttachmentStoreOp VkCast(AttachmentStoreOp storeOp)
{
    switch (storeOp)
    {
    case AttachmentStoreOp::Store:
        return ::vk::AttachmentStoreOp::eStore;
    case AttachmentStoreOp::DontCare:
        return ::vk::AttachmentStoreOp::eDontCare;
    default:
        APH_ASSERT(false && "Unhandled AttachmentStoreOp in VkCast");
        return ::vk::AttachmentStoreOp::eDontCare;
    }
}

::vk::ClearValue VkCast(const ClearValue& clearValue)
{
    ::vk::ClearValue vkClearValue;
    std::memcpy(&vkClearValue, &clearValue, sizeof(::vk::ClearValue));
    return vkClearValue;
}

::vk::Rect2D VkCast(const Rect2D& rect)
{
    ::vk::Rect2D vkRect;
    vkRect.offset = ::vk::Offset2D{ rect.offset.x, rect.offset.y };
    vkRect.extent = ::vk::Extent2D{ rect.extent.width, rect.extent.height };
    return vkRect;
}

::vk::Offset3D VkCast(const Offset3D& offset)
{
    return ::vk::Offset3D(offset.x, offset.y, offset.z);
}

::vk::ImageSubresourceLayers VkCast(const ImageSubresourceLayers& subresourceLayers)
{
    ::vk::ImageSubresourceLayers vkSubresourceLayers;
    vkSubresourceLayers.aspectMask = static_cast<::vk::ImageAspectFlags>(subresourceLayers.aspectMask);
    vkSubresourceLayers.mipLevel = subresourceLayers.mipLevel;
    vkSubresourceLayers.baseArrayLayer = subresourceLayers.baseArrayLayer;
    vkSubresourceLayers.layerCount = subresourceLayers.layerCount;
    return vkSubresourceLayers;
}

::vk::BufferImageCopy VkCast(const BufferImageCopy& bufferImageCopy)
{
    ::vk::BufferImageCopy vkBufferImageCopy;
    vkBufferImageCopy.bufferOffset = bufferImageCopy.bufferOffset;
    vkBufferImageCopy.bufferRowLength = bufferImageCopy.bufferRowLength;
    vkBufferImageCopy.bufferImageHeight = bufferImageCopy.bufferImageHeight;
    vkBufferImageCopy.imageSubresource = VkCast(bufferImageCopy.imageSubresource);
    vkBufferImageCopy.imageOffset = VkCast(bufferImageCopy.imageOffset);
    vkBufferImageCopy.imageExtent = ::vk::Extent3D(
        bufferImageCopy.imageExtent.width, bufferImageCopy.imageExtent.height, bufferImageCopy.imageExtent.depth);
    return vkBufferImageCopy;
}

::vk::PipelineStageFlagBits VkCast(PipelineStage stage)
{
    switch (stage)
    {
    case PipelineStage::TopOfPipe:
        return ::vk::PipelineStageFlagBits::eTopOfPipe;
    case PipelineStage::DrawIndirect:
        return ::vk::PipelineStageFlagBits::eDrawIndirect;
    case PipelineStage::VertexInput:
        return ::vk::PipelineStageFlagBits::eVertexInput;
    case PipelineStage::VertexShader:
        return ::vk::PipelineStageFlagBits::eVertexShader;
    case PipelineStage::TessellationControl:
        return ::vk::PipelineStageFlagBits::eTessellationControlShader;
    case PipelineStage::TessellationEvaluation:
        return ::vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case PipelineStage::GeometryShader:
        return ::vk::PipelineStageFlagBits::eGeometryShader;
    case PipelineStage::FragmentShader:
        return ::vk::PipelineStageFlagBits::eFragmentShader;
    case PipelineStage::EarlyFragmentTests:
        return ::vk::PipelineStageFlagBits::eEarlyFragmentTests;
    case PipelineStage::LateFragmentTests:
        return ::vk::PipelineStageFlagBits::eLateFragmentTests;
    case PipelineStage::ColorAttachmentOutput:
        return ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case PipelineStage::ComputeShader:
        return ::vk::PipelineStageFlagBits::eComputeShader;
    case PipelineStage::Transfer:
        return ::vk::PipelineStageFlagBits::eTransfer;
    case PipelineStage::BottomOfPipe:
        return ::vk::PipelineStageFlagBits::eBottomOfPipe;
    case PipelineStage::Host:
        return ::vk::PipelineStageFlagBits::eHost;
    case PipelineStage::AllGraphics:
        return ::vk::PipelineStageFlagBits::eAllGraphics;
    case PipelineStage::AllCommands:
        return ::vk::PipelineStageFlagBits::eAllCommands;
    default:
        APH_ASSERT(false && "Unhandled PipelineStage in VkCast");
        return ::vk::PipelineStageFlagBits::eBottomOfPipe;
    }
}

ShaderStageFlags getShaderStages(::vk::ShaderStageFlags vkStages)
{
    ShaderStageFlags flags = {};
    
    if (vkStages & ::vk::ShaderStageFlagBits::eVertex)
        flags |= ShaderStage::VS;
    if (vkStages & ::vk::ShaderStageFlagBits::eTessellationControl)
        flags |= ShaderStage::TCS;
    if (vkStages & ::vk::ShaderStageFlagBits::eTessellationEvaluation)
        flags |= ShaderStage::TES;
    if (vkStages & ::vk::ShaderStageFlagBits::eGeometry)
        flags |= ShaderStage::GS;
    if (vkStages & ::vk::ShaderStageFlagBits::eFragment)
        flags |= ShaderStage::FS;
    if (vkStages & ::vk::ShaderStageFlagBits::eCompute)
        flags |= ShaderStage::CS;
    if (vkStages & ::vk::ShaderStageFlagBits::eTaskEXT)
        flags |= ShaderStage::TS;
    if (vkStages & ::vk::ShaderStageFlagBits::eMeshEXT)
        flags |= ShaderStage::MS;
    if (vkStages & ::vk::ShaderStageFlagBits::eAll)
        flags |= ShaderStage::All;
        
    return flags;
}

::vk::PushConstantRange VkCast(const aph::PushConstantRange& aphRange)
{
    return ::vk::PushConstantRange(VkCast(aphRange.stageFlags), aphRange.offset, aphRange.size);
}

PushConstantRange getPushConstantRange(const ::vk::PushConstantRange& vkRange)
{
    return aph::PushConstantRange{ .stageFlags = getShaderStages(vkRange.stageFlags),
                                   .offset = vkRange.offset,
                                   .size = vkRange.size };
}

uint32_t getFormatSize(Format format)
{
    // Calculate bytes per pixel based on format
    switch (format)
    {
    // 8-bit formats (1 byte per component)
    case Format::R8_UINT:
    case Format::R8_SINT:
    case Format::R8_UNORM:
    case Format::R8_SNORM:
        return 1;
        
    // 16-bit formats (2 bytes per component) and 8-bit 2-component formats
    case Format::R16_UINT:
    case Format::R16_SINT:
    case Format::R16_UNORM:
    case Format::R16_SNORM:
    case Format::R16_FLOAT:
    case Format::RG8_UINT:
    case Format::RG8_SINT:
    case Format::RG8_UNORM:
    case Format::RG8_SNORM:
    case Format::BGRA4_UNORM:
    case Format::B5G6R5_UNORM:
    case Format::B5G5R5A1_UNORM:
        return 2;
        
    // 32-bit formats (4 bytes per component) and 8-bit 4-component formats
    case Format::R32_UINT:
    case Format::R32_SINT:
    case Format::R32_FLOAT:
    case Format::RGBA8_UINT:
    case Format::RGBA8_SINT:
    case Format::RGBA8_UNORM:
    case Format::RGBA8_SNORM:
    case Format::BGRA8_UNORM:
    case Format::SRGBA8_UNORM:
    case Format::SBGRA8_UNORM:
    case Format::RGB8_UINT: // Padded to 4 bytes in many cases
    case Format::RGB8_SINT:
    case Format::RGB8_UNORM:
    case Format::RGB8_SNORM:
    case Format::R11G11B10_FLOAT:
    case Format::R10G10B10A2_UNORM:
    case Format::D24S8:
    case Format::X24G8_UINT:
    case Format::D32:
        return 4;
        
    // 64-bit formats
    case Format::RG32_UINT:
    case Format::RG32_SINT:
    case Format::RG32_FLOAT:
    case Format::RGBA16_UINT:
    case Format::RGBA16_SINT:
    case Format::RGBA16_UNORM:
    case Format::RGBA16_SNORM:
    case Format::RGBA16_FLOAT:
        return 8;
        
    // 96-bit formats (12 bytes)
    case Format::RGB32_UINT:
    case Format::RGB32_SINT:
    case Format::RGB32_FLOAT:
        return 12;
        
    // 128-bit formats (16 bytes)
    case Format::RGBA32_UINT:
    case Format::RGBA32_SINT:
    case Format::RGBA32_FLOAT:
        return 16;
        
    // Depth-stencil formats
    case Format::D16:
        return 2;
    case Format::D32S8:
    case Format::X32G8_UINT:
        return 8;
        
    // Block compressed formats - handled by bytes per block
    case Format::BC1_UNORM:
    case Format::BC1_UNORM_SRGB:
        return 8; // 8 bytes per 4x4 block (0.5 bytes per pixel)
        
    case Format::BC2_UNORM:
    case Format::BC2_UNORM_SRGB:
    case Format::BC3_UNORM:
    case Format::BC3_UNORM_SRGB:
    case Format::BC4_UNORM:
    case Format::BC4_SNORM:
    case Format::BC5_UNORM:
    case Format::BC5_SNORM:
    case Format::BC6H_UFLOAT:
    case Format::BC6H_SFLOAT:
    case Format::BC7_UNORM:
    case Format::BC7_UNORM_SRGB:
        return 16; // 16 bytes per 4x4 block (1 byte per pixel)
        
    // Default case
    case Format::Undefined:
    default:
        VK_LOG_WARN("Unknown format in getFormatSize, returning 4 bytes as default");
        return 4;
    }
}
} // namespace aph::vk::utils

namespace aph::vk
{

const ::vk::AllocationCallbacks& vk_allocator()
{
    // Lambdas for the Vulkan allocation callbacks:
    static auto vkAphAlloc = [](void* pUserData, size_t size, size_t alignment,
                                ::vk::SystemAllocationScope allocationScope) -> void*
    { return memory::aph_memalign(alignment, size); };

    static auto vkAphRealloc = [](void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                  ::vk::SystemAllocationScope allocationScope) -> void*
    { return memory::aph_realloc(pOriginal, size); };

    static auto vkAphFree = [](void* pUserData, void* pMemory) -> void { memory::aph_free(pMemory); };

    static ::vk::AllocationCallbacks allocator_hpp{ nullptr, vkAphAlloc, vkAphRealloc, vkAphFree };

    return allocator_hpp;
}

const VkAllocationCallbacks* vkAllocator()
{
    // Lambdas for the Vulkan allocation callbacks:
    auto vkAphAlloc = [](void* pUserData, size_t size, size_t alignment,
                         VkSystemAllocationScope allocationScope) -> void*
    { return memory::aph_memalign(alignment, size); };

    auto vkAphRealloc = [](void* pUserData, void* pOriginal, size_t size, size_t alignment,
                           VkSystemAllocationScope allocationScope) -> void*
    { return memory::aph_realloc(pOriginal, size); };

    auto vkAphFree = [](void* pUserData, void* pMemory) -> void { memory::aph_free(pMemory); };

    static const VkAllocationCallbacks allocator = {
        .pfnAllocation = vkAphAlloc,
        .pfnReallocation = vkAphRealloc,
        .pfnFree = vkAphFree,
    };
    return &allocator;
}
} // namespace aph::vk
