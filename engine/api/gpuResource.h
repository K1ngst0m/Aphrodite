#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"

namespace aph
{
enum BufferUsageFlagBits
{
    BUFFER_USAGE_TRANSFER_SRC_BIT          = 0x00000001,
    BUFFER_USAGE_TRANSFER_DST_BIT          = 0x00000002,
    BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT  = 0x00000004,
    BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT  = 0x00000008,
    BUFFER_USAGE_UNIFORM_BUFFER_BIT        = 0x00000010,
    BUFFER_USAGE_STORAGE_BUFFER_BIT        = 0x00000020,
    BUFFER_USAGE_INDEX_BUFFER_BIT          = 0x00000040,
    BUFFER_USAGE_VERTEX_BUFFER_BIT         = 0x00000080,
    BUFFER_USAGE_INDIRECT_BUFFER_BIT       = 0x00000100,
    BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT = 0x00020000,
    BUFFER_USAGE_FLAG_BITS_MAX_ENUM        = 0x7FFFFFFF
};
using BufferUsageFlags = uint32_t;

enum ImageUsageFlagBits
{
    IMAGE_USAGE_TRANSFER_SRC_BIT             = 0x00000001,
    IMAGE_USAGE_TRANSFER_DST_BIT             = 0x00000002,
    IMAGE_USAGE_SAMPLED_BIT                  = 0x00000004,
    IMAGE_USAGE_STORAGE_BIT                  = 0x00000008,
    IMAGE_USAGE_COLOR_ATTACHMENT_BIT         = 0x00000010,
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT     = 0x00000040,
    IMAGE_USAGE_INPUT_ATTACHMENT_BIT         = 0x00000080,
    IMAGE_USAGE_FLAG_BITS_MAX_ENUM           = 0x7FFFFFFF
};
using ImageUsageFlags = uint32_t;

enum MemoryPropertyFlagBits
{
    MEMORY_PROPERTY_DEVICE_LOCAL_BIT     = 0x00000001,
    MEMORY_PROPERTY_HOST_VISIBLE_BIT     = 0x00000002,
    MEMORY_PROPERTY_HOST_COHERENT_BIT    = 0x00000004,
    MEMORY_PROPERTY_HOST_CACHED_BIT      = 0x00000008,
    MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = 0x00000010,
    MEMORY_PROPERTY_PROTECTED_BIT        = 0x00000020,
    MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM   = 0x7FFFFFFF
};
using MemoryPropertyFlags = uint32_t;

enum SampleCountFlagBits
{
    SAMPLE_COUNT_1_BIT              = 0x00000001,
    SAMPLE_COUNT_2_BIT              = 0x00000002,
    SAMPLE_COUNT_4_BIT              = 0x00000004,
    SAMPLE_COUNT_8_BIT              = 0x00000008,
    SAMPLE_COUNT_16_BIT             = 0x00000010,
    SAMPLE_COUNT_32_BIT             = 0x00000020,
    SAMPLE_COUNT_64_BIT             = 0x00000040,
    SAMPLE_COUNT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
};
using SampleCountFlags = uint32_t;

enum class ShaderStage
{
    VS  = 0,
    TCS = 1,
    TES = 2,
    GS  = 3,
    FS  = 4,
    CS  = 5,
    TS  = 6,
    MS  = 7,
};

enum class ImageTiling
{
    OPTIMAL = 0,
    LINEAR  = 1,
};

enum class ImageViewType
{
    _1D         = 0,
    _2D         = 1,
    _3D         = 2,
    _CUBE       = 3,
    _1D_ARRAY   = 4,
    _2D_ARRAY   = 5,
    _CUBE_ARRAY = 6,
};

enum class ImageViewDimension
{
    _1D         = 0,
    _2D         = 1,
    _3D         = 2,
    _CUBE       = 3,
    _1D_ARRAY   = 4,
    _2D_ARRAY   = 5,
    _CUBE_ARRAY = 6,
};

enum class ImageType
{
    _1D = 0,
    _2D = 1,
    _3D = 2,
};

enum class ImageLayout
{
    UNDEFINED                        = 0,
    GENERAL                          = 1,
    COLOR_ATTACHMENT_OPTIMAL         = 2,
    DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    DEPTH_STENCIL_READ_ONLY_OPTIMAL  = 4,
    SHADER_READ_ONLY_OPTIMAL         = 5,
    TRANSFER_SRC_OPTIMAL             = 6,
    TRANSFER_DST_OPTIMAL             = 7,
    PREINITIALIZED                   = 8,
};

enum class ResourceType
{
    UNDEFINED = 0,
    SAMPLER = 1,
    SAMPLED_IMAGE = 2,
    COMBINE_SAMPLER_IMAGE = 3,
    STORAGE_IMAGE = 4,
    UNIFORM_BUFFER = 5,
    STORAGE_BUFFER = 6,
};

enum class ComponentSwizzle
{
    IDENTITY = 0,
    ZERO     = 1,
    ONE      = 2,
    R        = 3,
    G        = 4,
    B        = 5,
    A        = 6,
};

struct ComponentMapping
{
    ComponentSwizzle r = {ComponentSwizzle::IDENTITY};
    ComponentSwizzle g = {ComponentSwizzle::IDENTITY};
    ComponentSwizzle b = {ComponentSwizzle::IDENTITY};
    ComponentSwizzle a = {ComponentSwizzle::IDENTITY};
};

struct ImageSubresourceRange
{
    uint32_t baseMipLevel   = {0};
    uint32_t levelCount     = {1};
    uint32_t baseArrayLayer = {0};
    uint32_t layerCount     = {1};
};

struct Extent3D
{
    uint32_t width  = {0};
    uint32_t height = {0};
    uint32_t depth  = {0};
};

struct DummyCreateInfo
{
    uint32_t typeId;
};

template <typename T_Handle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
public:
    ResourceHandle()
    {
        if constexpr(std::is_same<T_CreateInfo, DummyCreateInfo>::value)
        {
            m_createInfo.typeId = typeid(T_Handle).hash_code();
        }
    }
    operator T_Handle() { return m_handle; }
    operator T_Handle&() { return m_handle; }
    operator T_Handle&() const { return m_handle; }

    T_Handle&       getHandle() { return m_handle; }
    const T_Handle& getHandle() const { return m_handle; }
    T_CreateInfo&   getCreateInfo() { return m_createInfo; }

protected:
    T_Handle     m_handle     = {};
    T_CreateInfo m_createInfo = {};
};

}  // namespace aph

#endif  // RESOURCE_H_
