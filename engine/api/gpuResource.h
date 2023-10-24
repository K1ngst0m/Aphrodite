#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"
#include "tinyimageformat.h"

namespace aph
{
enum class BufferDomain : uint8_t
{
    Device,                            // Device local. Probably not visible from CPU.
    LinkedDeviceHost,                  // On desktop, directly mapped VRAM over PCI.
    LinkedDeviceHostPreferDevice,      // Prefer device local of host visible.
    Host,                              // Host-only, needs to be synced to GPU. Might be device local as well on iGPUs.
    CachedHost,                        //
    CachedCoherentHostPreferCoherent,  // Aim for both cached and coherent, but prefer COHERENT
    CachedCoherentHostPreferCached,    // Aim for both cached and coherent, but prefer CACHED
};

enum class ImageDomain : uint8_t
{
    Device,
    Transient,
    LinearHostCached,
    LinearHost
};

enum class QueueType : uint8_t
{
    GRAPHICS = 0,
    COMPUTE  = 1,
    TRANSFER = 2,
};

enum class ShaderStage : uint8_t
{
    NA  = 0,
    VS  = 1,
    TCS = 2,
    TES = 3,
    GS  = 4,
    FS  = 5,
    CS  = 6,
    TS  = 7,
    MS  = 8,
};

enum class ResourceType : uint8_t
{
    Undefined           = 0,
    Sampler             = 1,
    SampledImage        = 2,
    CombineSamplerImage = 3,
    StorageImage        = 4,
    UniformBuffer       = 5,
    StorageBuffer       = 6,
};

enum ResourceState
{
    RESOURCE_STATE_UNDEFINED                         = 0,
    RESOURCE_STATE_VERTEX_BUFFER                     = 0x1,
    RESOURCE_STATE_UNIFORM_BUFFER                    = 0x2,
    RESOURCE_STATE_INDEX_BUFFER                      = 0x4,
    RESOURCE_STATE_RENDER_TARGET                     = 0x8,
    RESOURCE_STATE_UNORDERED_ACCESS                  = 0x10,
    RESOURCE_STATE_DEPTH_STENCIL                     = 0x20,
    RESOURCE_STATE_SHADER_RESOURCE                   = 0x40,
    RESOURCE_STATE_STREAM_OUT                        = 0x80,
    RESOURCE_STATE_INDIRECT_ARGUMENT                 = 0x100,
    RESOURCE_STATE_COPY_DST                          = 0x200,
    RESOURCE_STATE_COPY_SRC                          = 0x400,
    RESOURCE_STATE_GENERIC_READ                      = ((((((0x1 | 0x2) | 0x4) | 0x40) | 0x100) | 0x400) | 0x800),
    RESOURCE_STATE_PRESENT                           = 0x800,
    RESOURCE_STATE_GENERAL                           = 0x1000,
    RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x2000,
    RESOURCE_STATE_SHADING_RATE_SOURCE               = 0x4000,
};

enum class SamplerPreset : uint8_t
{
    Nearest,
    Linear,
    Anisotropic,
    Mipmap,
};

struct Extent2D
{
    uint32_t width  = {0};
    uint32_t height = {0};
};

struct Extent3D
{
    uint32_t width  = {0};
    uint32_t height = {0};
    uint32_t depth  = {0};
};

struct MemoryRange
{
    std::size_t offset = {0};
    std::size_t size   = {0};
};

struct DebugLabel
{
    std::string name;
    float       color[4];
};

struct ShaderMacro
{
    std::string definition;
    std::string value;
};

struct DummyCreateInfo
{
    uint32_t typeId;
};

enum class Format : uint8_t
{
    Undefined = 0,

    R_UN8,
    R_UI16,
    R_UN16,
    R_F16,
    R_F32,

    RG_UN8,
    RG_UI16,
    RG_UN16,
    RG_F16,
    RG_F32,

    RGB_UN8,
    RGB_UI32,
    RGB_F16,
    RGB_F32,

    RGBA_UN8,
    RGBA_UI32,
    RGBA_F16,
    RGBA_F32,
    RGBA_SRGB8,

    BGRA_UN8,
    BGRA_SRGB8,

    ETC2_RGB8,
    ETC2_SRGB8,
    BC7_RGBA,

    Z_UN16,
    Z_UN24,
    Z_F32,
    Z_UN24_S_UI8,
    Z_F32_S_UI8,
};

enum WaveOpsSupportFlags
{
    WAVE_OPS_SUPPORT_FLAG_NONE                 = 0x0,
    WAVE_OPS_SUPPORT_FLAG_BASIC_BIT            = 0x00000001,
    WAVE_OPS_SUPPORT_FLAG_VOTE_BIT             = 0x00000002,
    WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT       = 0x00000004,
    WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT           = 0x00000008,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT          = 0x00000010,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT = 0x00000020,
    WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT        = 0x00000040,
    WAVE_OPS_SUPPORT_FLAG_QUAD_BIT             = 0x00000080,
    WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV   = 0x00000100,
    WAVE_OPS_SUPPORT_FLAG_ALL                  = 0x7FFFFFFF
};
MAKE_ENUM_FLAG(uint32_t, WaveOpsSupportFlags);

constexpr uint32_t MAX_GPU_VENDOR_STRING_LENGTH = 256;  // max size for GPUVendorPreset strings

struct GPUVendorPreset
{
    std::string vendorId;
    std::string modelId;
    std::string revisionId;
    std::string gpuName;
    std::string gpuDriverVersion;
    std::string gpuDriverDate;
    uint32_t    rtCoresCount;
};

struct GPUSettings
{
    uint64_t            vram;
    uint32_t            uniformBufferAlignment;
    uint32_t            uploadBufferTextureAlignment;
    uint32_t            uploadBufferTextureRowAlignment;
    uint32_t            maxVertexInputBindings;
    uint32_t            maxRootSignatureDWORDS;
    uint32_t            waveLaneCount;
    WaveOpsSupportFlags waveOpsSupportFlags;
    GPUVendorPreset     GpuVendorPreset;

    uint8_t multiDrawIndirect : 1;
    uint8_t indirectRootConstant : 1;
    uint8_t builtinDrawID : 1;
    uint8_t indirectCommandBuffer : 1;
    uint8_t rovsSupported : 1;
    uint8_t tessellationSupported : 1;
    uint8_t geometryShaderSupported : 1;
    uint8_t gpuBreadcrumbs : 1;
    uint8_t hdrSupported : 1;
    uint8_t samplerAnisotropySupported : 1;
};

struct DrawArguments
{
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct DrawIndexArguments
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t vertexOffset;
    uint32_t firstInstance;
};

struct DispatchArguments
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

enum class IndexType
{
    NONE,
    UINT16,
    UINT32,
};

enum class PrimitiveTopology
{
    TRI_LIST,
    TRI_STRIP,
};

struct VertexInput
{
    struct VertexAttribute
    {
        uint32_t location = 0;
        uint32_t binding  = 0;
        Format   format   = Format::Undefined;
        uint32_t offset   = 0;
    };

    struct VertexInputBinding
    {
        uint32_t stride = 0;
    };

    std::vector<VertexAttribute>    attributes;
    std::vector<VertexInputBinding> bindings;
};

template <typename T_Handle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle : public std::true_type
{
public:
    using HandleType     = T_Handle;
    using CreateInfoType = T_CreateInfo;

    ResourceHandle(HandleType handle, CreateInfoType createInfo = DummyCreateInfo{}) :
        m_handle(handle),
        m_createInfo(createInfo)
    {
        if constexpr(std::is_same_v<T_CreateInfo, DummyCreateInfo>)
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

namespace aph::utils
{
constexpr inline uint32_t getFormatSize(Format format)
{
    switch(format)
    {
    case Format::Undefined:
        return 0;

    case Format::R_UN8:
        return 1;
    case Format::R_UI16:
        return 2;
    case Format::R_UN16:
        return 2;
    case Format::R_F16:
        return 2;
    case Format::R_F32:
        return 4;

    case Format::RG_UN8:
        return 2;
    case Format::RG_UI16:
        return 4;
    case Format::RG_UN16:
        return 4;
    case Format::RG_F16:
        return 4;
    case Format::RG_F32:
        return 8;

    case Format::RGB_UN8:
        return 3;
    case Format::RGB_UI32:
        return 12;
    case Format::RGB_F16:
        return 6;
    case Format::RGB_F32:
        return 12;

    case Format::RGBA_UN8:
        return 4;
    case Format::RGBA_UI32:
        return 16;
    case Format::RGBA_F16:
        return 8;
    case Format::RGBA_F32:
        return 16;
    case Format::RGBA_SRGB8:
        return 4;

    case Format::BGRA_UN8:
        return 4;
    case Format::BGRA_SRGB8:
        return 4;

    // TODO assuming a block size of 4x4
    case Format::ETC2_RGB8:
        return 8;
    case Format::ETC2_SRGB8:
        return 8;
    case Format::BC7_RGBA:
        return 16;

    case Format::Z_UN16:
        return 2;
    case Format::Z_UN24:
        return 3;
    case Format::Z_F32:
        return 4;
    case Format::Z_UN24_S_UI8:
        return 4;  // 3 bytes for depth, 1 byte for stencil
    case Format::Z_F32_S_UI8:
        return 5;  // 4 bytes for depth, 1 byte for stencil

    default:
        return 0;
    }
}

}  // namespace aph::utils

#endif  // RESOURCE_H_
