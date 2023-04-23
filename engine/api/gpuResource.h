#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"

namespace aph
{
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
    UNDEFINED                = 0,
    GENERAL                  = 1,
    COLOR_ATTACHMENT         = 2,
    DEPTH_STENCIL_ATTACHMENT = 3,
    DEPTH_STENCIL_RO         = 4,
    SHADER_RO                = 5,
    TRANSFER_SRC             = 6,
    TRANSFER_DST             = 7,
    PRESENT_SRC              = 8,
};

enum class ResourceType
{
    UNDEFINED             = 0,
    SAMPLER               = 1,
    SAMPLED_IMAGE         = 2,
    COMBINE_SAMPLER_IMAGE = 3,
    STORAGE_IMAGE         = 4,
    UNIFORM_BUFFER        = 5,
    STORAGE_BUFFER        = 6,
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
