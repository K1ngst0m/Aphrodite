#include "vkUtils.h"
#include "common/hash.h"
#include "volk.h"

#include "allocator/allocator.h"

namespace aph::vk::utils
{
struct FormatMapping
{
    Format   rhiFormat;
    VkFormat vkFormat;
};

static const std::array<FormatMapping, size_t(Format::COUNT)> FormatMap = {{
    {Format::Undefined, VK_FORMAT_UNDEFINED},
    {Format::R8_UINT, VK_FORMAT_R8_UINT},
    {Format::R8_SINT, VK_FORMAT_R8_SINT},
    {Format::R8_UNORM, VK_FORMAT_R8_UNORM},
    {Format::R8_SNORM, VK_FORMAT_R8_SNORM},
    {Format::RG8_UINT, VK_FORMAT_R8G8_UINT},
    {Format::RG8_SINT, VK_FORMAT_R8G8_SINT},
    {Format::RG8_UNORM, VK_FORMAT_R8G8_UNORM},
    {Format::RG8_SNORM, VK_FORMAT_R8G8_SNORM},
    {Format::RGB8_UINT, VK_FORMAT_R8G8B8_UINT},
    {Format::RGB8_SINT, VK_FORMAT_R8G8B8_SINT},
    {Format::RGB8_UNORM, VK_FORMAT_R8G8B8_UNORM},
    {Format::RGB8_SNORM, VK_FORMAT_R8G8B8_SNORM},
    {Format::R16_UINT, VK_FORMAT_R16_UINT},
    {Format::R16_SINT, VK_FORMAT_R16_SINT},
    {Format::R16_UNORM, VK_FORMAT_R16_UNORM},
    {Format::R16_SNORM, VK_FORMAT_R16_SNORM},
    {Format::R16_FLOAT, VK_FORMAT_R16_SFLOAT},
    {Format::BGRA4_UNORM, VK_FORMAT_B4G4R4A4_UNORM_PACK16},
    {Format::B5G6R5_UNORM, VK_FORMAT_B5G6R5_UNORM_PACK16},
    {Format::B5G5R5A1_UNORM, VK_FORMAT_B5G5R5A1_UNORM_PACK16},
    {Format::RGBA8_UINT, VK_FORMAT_R8G8B8A8_UINT},
    {Format::RGBA8_SINT, VK_FORMAT_R8G8B8A8_SINT},
    {Format::RGBA8_UNORM, VK_FORMAT_R8G8B8A8_UNORM},
    {Format::RGBA8_SNORM, VK_FORMAT_R8G8B8A8_SNORM},
    {Format::BGRA8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
    {Format::SRGBA8_UNORM, VK_FORMAT_R8G8B8A8_SRGB},
    {Format::SBGRA8_UNORM, VK_FORMAT_B8G8R8A8_SRGB},
    {Format::R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {Format::R11G11B10_FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32},
    {Format::RG16_UINT, VK_FORMAT_R16G16_UINT},
    {Format::RG16_SINT, VK_FORMAT_R16G16_SINT},
    {Format::RG16_UNORM, VK_FORMAT_R16G16_UNORM},
    {Format::RG16_SNORM, VK_FORMAT_R16G16_SNORM},
    {Format::RG16_FLOAT, VK_FORMAT_R16G16_SFLOAT},
    {Format::RGB16_UINT, VK_FORMAT_R16G16B16_UINT},
    {Format::RGB16_SINT, VK_FORMAT_R16G16B16_SINT},
    {Format::RGB16_UNORM, VK_FORMAT_R16G16B16_UNORM},
    {Format::RGB16_SNORM, VK_FORMAT_R16G16B16_SNORM},
    {Format::RGB16_FLOAT, VK_FORMAT_R16G16B16_SFLOAT},
    {Format::R32_UINT, VK_FORMAT_R32_UINT},
    {Format::R32_SINT, VK_FORMAT_R32_SINT},
    {Format::R32_FLOAT, VK_FORMAT_R32_SFLOAT},
    {Format::RGBA16_UINT, VK_FORMAT_R16G16B16A16_UINT},
    {Format::RGBA16_SINT, VK_FORMAT_R16G16B16A16_SINT},
    {Format::RGBA16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT},
    {Format::RGBA16_UNORM, VK_FORMAT_R16G16B16A16_UNORM},
    {Format::RGBA16_SNORM, VK_FORMAT_R16G16B16A16_SNORM},
    {Format::RG32_UINT, VK_FORMAT_R32G32_UINT},
    {Format::RG32_SINT, VK_FORMAT_R32G32_SINT},
    {Format::RG32_FLOAT, VK_FORMAT_R32G32_SFLOAT},
    {Format::RGB32_UINT, VK_FORMAT_R32G32B32_UINT},
    {Format::RGB32_SINT, VK_FORMAT_R32G32B32_SINT},
    {Format::RGB32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT},
    {Format::RGBA32_UINT, VK_FORMAT_R32G32B32A32_UINT},
    {Format::RGBA32_SINT, VK_FORMAT_R32G32B32A32_SINT},
    {Format::RGBA32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
    {Format::D16, VK_FORMAT_D16_UNORM},
    {Format::D24S8, VK_FORMAT_D24_UNORM_S8_UINT},
    {Format::X24G8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    {Format::D32, VK_FORMAT_D32_SFLOAT},
    {Format::D32S8, VK_FORMAT_D32_SFLOAT_S8_UINT},
    {Format::X32G8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT},
    {Format::BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {Format::BC1_UNORM_SRGB, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
    {Format::BC2_UNORM, VK_FORMAT_BC2_UNORM_BLOCK},
    {Format::BC2_UNORM_SRGB, VK_FORMAT_BC2_SRGB_BLOCK},
    {Format::BC3_UNORM, VK_FORMAT_BC3_UNORM_BLOCK},
    {Format::BC3_UNORM_SRGB, VK_FORMAT_BC3_SRGB_BLOCK},
    {Format::BC4_UNORM, VK_FORMAT_BC4_UNORM_BLOCK},
    {Format::BC4_SNORM, VK_FORMAT_BC4_SNORM_BLOCK},
    {Format::BC5_UNORM, VK_FORMAT_BC5_UNORM_BLOCK},
    {Format::BC5_SNORM, VK_FORMAT_BC5_SNORM_BLOCK},
    {Format::BC6H_UFLOAT, VK_FORMAT_BC6H_UFLOAT_BLOCK},
    {Format::BC6H_SFLOAT, VK_FORMAT_BC6H_SFLOAT_BLOCK},
    {Format::BC7_UNORM, VK_FORMAT_BC7_UNORM_BLOCK},
    {Format::BC7_UNORM_SRGB, VK_FORMAT_BC7_SRGB_BLOCK},

}};
static const HashMap<VkFormat, Format> VkToAphFormatMap = {
    {VK_FORMAT_UNDEFINED,                Format::Undefined},
    {VK_FORMAT_R8_UINT,                  Format::R8_UINT},
    {VK_FORMAT_R8_SINT,                  Format::R8_SINT},
    {VK_FORMAT_R8_UNORM,                 Format::R8_UNORM},
    {VK_FORMAT_R8_SNORM,                 Format::R8_SNORM},
    {VK_FORMAT_R8G8_UINT,                Format::RG8_UINT},
    {VK_FORMAT_R8G8_SINT,                Format::RG8_SINT},
    {VK_FORMAT_R8G8_UNORM,               Format::RG8_UNORM},
    {VK_FORMAT_R8G8_SNORM,               Format::RG8_SNORM},
    {VK_FORMAT_R8G8B8_UINT,              Format::RGB8_UINT},
    {VK_FORMAT_R8G8B8_SINT,              Format::RGB8_SINT},
    {VK_FORMAT_R8G8B8_UNORM,             Format::RGB8_UNORM},
    {VK_FORMAT_R8G8B8_SNORM,             Format::RGB8_SNORM},
    {VK_FORMAT_R16_UINT,                 Format::R16_UINT},
    {VK_FORMAT_R16_SINT,                 Format::R16_SINT},
    {VK_FORMAT_R16_UNORM,                Format::R16_UNORM},
    {VK_FORMAT_R16_SNORM,                Format::R16_SNORM},
    {VK_FORMAT_R16_SFLOAT,               Format::R16_FLOAT},
    {VK_FORMAT_B4G4R4A4_UNORM_PACK16,    Format::BGRA4_UNORM},
    {VK_FORMAT_B5G6R5_UNORM_PACK16,      Format::B5G6R5_UNORM},
    {VK_FORMAT_B5G5R5A1_UNORM_PACK16,    Format::B5G5R5A1_UNORM},
    {VK_FORMAT_R8G8B8A8_UINT,            Format::RGBA8_UINT},
    {VK_FORMAT_R8G8B8A8_SINT,            Format::RGBA8_SINT},
    {VK_FORMAT_R8G8B8A8_UNORM,           Format::RGBA8_UNORM},
    {VK_FORMAT_R8G8B8A8_SNORM,           Format::RGBA8_SNORM},
    {VK_FORMAT_B8G8R8A8_UNORM,           Format::BGRA8_UNORM},
    {VK_FORMAT_R8G8B8A8_SRGB,            Format::SRGBA8_UNORM},
    {VK_FORMAT_B8G8R8A8_SRGB,            Format::SBGRA8_UNORM},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32, Format::R10G10B10A2_UNORM},
    {VK_FORMAT_B10G11R11_UFLOAT_PACK32,  Format::R11G11B10_FLOAT},
    {VK_FORMAT_R16G16_UINT,              Format::RG16_UINT},
    {VK_FORMAT_R16G16_SINT,              Format::RG16_SINT},
    {VK_FORMAT_R16G16_UNORM,             Format::RG16_UNORM},
    {VK_FORMAT_R16G16_SNORM,             Format::RG16_SNORM},
    {VK_FORMAT_R16G16_SFLOAT,            Format::RG16_FLOAT},
    {VK_FORMAT_R16G16B16_UINT,           Format::RGB16_UINT},
    {VK_FORMAT_R16G16B16_SINT,           Format::RGB16_SINT},
    {VK_FORMAT_R16G16B16_UNORM,          Format::RGB16_UNORM},
    {VK_FORMAT_R16G16B16_SNORM,          Format::RGB16_SNORM},
    {VK_FORMAT_R16G16B16_SFLOAT,         Format::RGB16_FLOAT},
    {VK_FORMAT_R32_UINT,                 Format::R32_UINT},
    {VK_FORMAT_R32_SINT,                 Format::R32_SINT},
    {VK_FORMAT_R32_SFLOAT,               Format::R32_FLOAT},
    {VK_FORMAT_R16G16B16A16_UINT,        Format::RGBA16_UINT},
    {VK_FORMAT_R16G16B16A16_SINT,        Format::RGBA16_SINT},
    {VK_FORMAT_R16G16B16A16_SFLOAT,      Format::RGBA16_FLOAT},
    {VK_FORMAT_R16G16B16A16_UNORM,       Format::RGBA16_UNORM},
    {VK_FORMAT_R16G16B16A16_SNORM,       Format::RGBA16_SNORM},
    {VK_FORMAT_R32G32_UINT,              Format::RG32_UINT},
    {VK_FORMAT_R32G32_SINT,              Format::RG32_SINT},
    {VK_FORMAT_R32G32_SFLOAT,            Format::RG32_FLOAT},
    {VK_FORMAT_R32G32B32_UINT,           Format::RGB32_UINT},
    {VK_FORMAT_R32G32B32_SINT,           Format::RGB32_SINT},
    {VK_FORMAT_R32G32B32_SFLOAT,         Format::RGB32_FLOAT},
    {VK_FORMAT_R32G32B32A32_UINT,        Format::RGBA32_UINT},
    {VK_FORMAT_R32G32B32A32_SINT,        Format::RGBA32_SINT},
    {VK_FORMAT_R32G32B32A32_SFLOAT,      Format::RGBA32_FLOAT},
    {VK_FORMAT_D16_UNORM,                Format::D16},
    {VK_FORMAT_D24_UNORM_S8_UINT,        Format::D24S8},  // Chosen over X24G8_UINT
    {VK_FORMAT_D32_SFLOAT,               Format::D32},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,       Format::D32S8},  // Chosen over X32G8_UINT
    {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,     Format::BC1_UNORM},
    {VK_FORMAT_BC1_RGBA_SRGB_BLOCK,      Format::BC1_UNORM_SRGB},
    {VK_FORMAT_BC2_UNORM_BLOCK,          Format::BC2_UNORM},
    {VK_FORMAT_BC2_SRGB_BLOCK,           Format::BC2_UNORM_SRGB},
    {VK_FORMAT_BC3_UNORM_BLOCK,          Format::BC3_UNORM},
    {VK_FORMAT_BC3_SRGB_BLOCK,           Format::BC3_UNORM_SRGB},
    {VK_FORMAT_BC4_UNORM_BLOCK,          Format::BC4_UNORM},
    {VK_FORMAT_BC4_SNORM_BLOCK,          Format::BC4_SNORM},
    {VK_FORMAT_BC5_UNORM_BLOCK,          Format::BC5_UNORM},
    {VK_FORMAT_BC5_SNORM_BLOCK,          Format::BC5_SNORM},
    {VK_FORMAT_BC6H_UFLOAT_BLOCK,        Format::BC6H_UFLOAT},
    {VK_FORMAT_BC6H_SFLOAT_BLOCK,        Format::BC6H_SFLOAT},
    {VK_FORMAT_BC7_UNORM_BLOCK,          Format::BC7_UNORM},
    {VK_FORMAT_BC7_SRGB_BLOCK,           Format::BC7_UNORM_SRGB},
};

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

VkAccessFlags getAccessFlags(ResourceState state)
{
    VkAccessFlags ret = 0;
    if((state & ResourceState::CopySource) != 0)
    {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if((state & ResourceState::CopyDest) != 0)
    {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if((state & ResourceState::VertexBuffer) != 0)
    {
        ret |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if((state & ResourceState::UniformBuffer) != 0)
    {
        ret |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    if((state & ResourceState::IndexBuffer) != 0)
    {
        ret |= VK_ACCESS_INDEX_READ_BIT;
    }
    if((state & ResourceState::UnorderedAccess) != 0)
    {
        ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if((state & ResourceState::IndirectArgument) != 0)
    {
        ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if((state & ResourceState::RenderTarget) != 0)
    {
        ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if((state & ResourceState::DepthStencil) != 0)
    {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if((state & ResourceState::ShaderResource) != 0)
    {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if((state & ResourceState::Present) != 0)
    {
        ret |= VK_ACCESS_MEMORY_READ_BIT;
    }
    if((state & ResourceState::AccelStructRead) != 0)
    {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    }
    if((state & ResourceState::AccelStructWrite) != 0)
    {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    }
    return ret;
}

VkImageLayout getImageLayout(ResourceState state)
{
    if((state & ResourceState::CopySource) != 0)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if((state & ResourceState::CopyDest) != 0)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if((state & ResourceState::RenderTarget) != 0)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if((state & ResourceState::DepthStencil) != 0)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if((state & ResourceState::UnorderedAccess) != 0)
        return VK_IMAGE_LAYOUT_GENERAL;

    if((state & ResourceState::ShaderResource) != 0)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if((state & ResourceState::Present) != 0)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if((state & ResourceState::General) != 0)
        return VK_IMAGE_LAYOUT_GENERAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkFormat VkCast(Format format)
{
    APH_ASSERT(format < Format::COUNT);
    APH_ASSERT(FormatMap[uint32_t(format)].rhiFormat == format);

    return FormatMap[uint32_t(format)].vkFormat;
}

Format getFormatFromVk(VkFormat format)
{
    APH_ASSERT(VkToAphFormatMap.contains(format));
    return VkToAphFormatMap.at(format);
}

Result getResult(VkResult result)
{
    switch(result)
    {
    case VK_SUCCESS:
        return Result::Success;
    default:
        return Result::RuntimeError;
    }
}
VkIndexType VkCast(IndexType indexType)
{
    switch(indexType)
    {
    case IndexType::UINT16:
        return VK_INDEX_TYPE_UINT16;
    case IndexType::UINT32:
        return VK_INDEX_TYPE_UINT32;
    default:
        APH_ASSERT(false);
        return VK_INDEX_TYPE_NONE_KHR;
    }
}

VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, std::string_view name)
{
#if APH_DEBUG
    if(name.empty())
    {
        return VK_SUCCESS;
    }
    const VkDebugUtilsObjectNameInfoEXT ni = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = type,
        .objectHandle = handle,
        .pObjectName  = name.data(),
    };
    return vkSetDebugUtilsObjectNameEXT(device, &ni);
#else
    return VK_SUCCESS;
#endif
}
VkImageAspectFlags getImageAspect(Format format)
{
    return getImageAspect(VkCast(format));
}

VkCompareOp VkCast(CompareOp compareOp)
{
    switch(compareOp)
    {
    case CompareOp::Never:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::Less:
        return VK_COMPARE_OP_LESS;
    case CompareOp::Equal:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::Greater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::Always:
        return VK_COMPARE_OP_ALWAYS;
    }

    APH_ASSERT(false);
    return VK_COMPARE_OP_ALWAYS;
}
VkPrimitiveTopology VkCast(PrimitiveTopology topology)
{
    switch(topology)
    {
    case PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    APH_ASSERT(false);
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkCullModeFlags VkCast(CullMode mode)
{
    switch(mode)
    {
    case CullMode::None:
        return VK_CULL_MODE_NONE;
    case CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    APH_ASSERT(false);
    return VK_CULL_MODE_NONE;
}

VkFrontFace VkCast(WindingMode mode)
{
    switch(mode)
    {
    case WindingMode::CCW:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case WindingMode::CW:
        return VK_FRONT_FACE_CLOCKWISE;
    }
    APH_ASSERT(false);
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

VkPolygonMode VkCast(PolygonMode mode)
{
    switch(mode)
    {
    case PolygonMode::Fill:
        return VK_POLYGON_MODE_FILL;
    case PolygonMode::Line:
        return VK_POLYGON_MODE_LINE;
    }
    APH_ASSERT(false);
    return VK_POLYGON_MODE_FILL;
}
VkBlendFactor VkCast(BlendFactor factor)
{
    switch(factor)
    {
    case BlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::SrcAlphaSaturated:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case BlendFactor::BlendColor:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case BlendFactor::OneMinusBlendColor:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor::BlendAlpha:
        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case BlendFactor::OneMinusBlendAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    case BlendFactor::Src1Color:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case BlendFactor::OneMinusSrc1Color:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case BlendFactor::Src1Alpha:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case BlendFactor::OneMinusSrc1Alpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    APH_ASSERT(false);
    return VK_BLEND_FACTOR_ZERO;
}
VkBlendOp VkCast(BlendOp op)
{
    switch(op)
    {
    case BlendOp::Add:
        return VK_BLEND_OP_ADD;
    case BlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min:
        return VK_BLEND_OP_MIN;
    case BlendOp::Max:
        return VK_BLEND_OP_MAX;
    }

    APH_ASSERT(false);
    return VK_BLEND_OP_ADD;
}
VkStencilOp VkCast(StencilOp op)
{
    switch(op)
    {
    case StencilOp::Keep:
        return VK_STENCIL_OP_KEEP;
    case StencilOp::Zero:
        return VK_STENCIL_OP_ZERO;
    case StencilOp::Replace:
        return VK_STENCIL_OP_REPLACE;
    case StencilOp::IncrementClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOp::DecrementClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case StencilOp::Invert:
        return VK_STENCIL_OP_INVERT;
    case StencilOp::IncrementWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case StencilOp::DecrementWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
    APH_ASSERT(false);
    return VK_STENCIL_OP_KEEP;
}
VkPipelineBindPoint VkCast(PipelineType type)
{
    switch(type)
    {
    case PipelineType::Geometry:
    case PipelineType::Mesh:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    case PipelineType::Compute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    case PipelineType::RayTracing:
        return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        break;
    default:
        APH_ASSERT(false);
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
        break;
    }
    APH_ASSERT(false);
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
}
}  // namespace aph::vk::utils

namespace aph::vk
{

const ::vk::AllocationCallbacks& vk_allocator()
{
    // Lambdas for the Vulkan allocation callbacks:
    auto vkAphAlloc = [](void* pUserData, size_t size, size_t alignment, ::vk::SystemAllocationScope allocationScope) -> void* {
        return memory::aph_memalign(alignment, size);
    };

    auto vkAphRealloc = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, ::vk::SystemAllocationScope allocationScope) -> void* {
        return memory::aph_realloc(pOriginal, size);
    };

    auto vkAphFree = [](void* pUserData, void* pMemory) -> void {
        memory::aph_free(pMemory);
    };

    static ::vk::AllocationCallbacks allocator {};
    allocator.setPfnAllocation(vkAphAlloc).setPfnFree(vkAphFree).setPfnReallocation(vkAphRealloc);
    return allocator;
}

const VkAllocationCallbacks* vkAllocator()
{
    // Lambdas for the Vulkan allocation callbacks:
    auto vkAphAlloc = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
        return memory::aph_memalign(alignment, size);
    };

    auto vkAphRealloc = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
        return memory::aph_realloc(pOriginal, size);
    };

    auto vkAphFree = [](void* pUserData, void* pMemory) -> void {
        memory::aph_free(pMemory);
    };

    static const VkAllocationCallbacks allocator = {
        .pfnAllocation   = vkAphAlloc,
        .pfnReallocation = vkAphRealloc,
        .pfnFree         = vkAphFree,
    };
    return &allocator;
}
}  // namespace aph::vk
