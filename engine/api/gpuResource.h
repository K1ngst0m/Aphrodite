#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"

namespace aph
{
enum class BufferDomain : uint8_t
{
    Device,            // Device local. Probably not visible from CPU.
    LinkedDeviceHost,  // On desktop, directly mapped VRAM over PCI.
    Host,              // Host-only, needs to be synced to GPU. Might be device local as well on iGPUs.
    CachedHost,        //
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
    Graphics = 0,
    Compute  = 1,
    Transfer = 2,
    Count,
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

enum class ResourceState : uint32_t
{
    Undefined        = 0,
    General          = 0x00000001,
    UniformBuffer    = 0x00000002,
    VertexBuffer     = 0x00000004,
    IndexBuffer      = 0x00000008,
    IndirectArgument = 0x00000010,
    ShaderResource   = 0x00000020,
    UnorderedAccess  = 0x00000040,
    RenderTarget     = 0x00000080,
    DepthStencil     = 0x00000100,
    StreamOut        = 0x00000200,
    CopyDest         = 0x00000400,
    CopySource       = 0x00000800,
    ResolveDest      = 0x00001000,
    ResolveSource    = 0x00002000,
    Present          = 0x00004000,
    AccelStructRead  = 0x00008000,
    AccelStructWrite = 0x00010000,
};
MAKE_ENUM_CLASS_FLAG(ResourceState);

enum class CompareOp : uint8_t
{
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class SamplerPreset : uint8_t
{
    NearestClamp,
    LinearClamp,
    TrilinearClamp,
    NearestWrap,
    LinearWrap,
    TrilinearWrap,
    NearestShadow,
    LinearShadow,
    DefaultGeometryFilterClamp,
    DefaultGeometryFilterWrap,
    Count
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

struct ShaderConstant
{
    const void* pValue;
    uint32_t    mIndex;
    uint32_t    mSize;
};

struct DepthState
{
    CompareOp compareOp;
    bool      enableWrite;
};

struct DummyCreateInfo
{
    uint32_t typeId;
};

enum class Format : uint8_t
{
    Undefined,

    R8_UINT,
    R8_SINT,
    R8_UNORM,
    R8_SNORM,
    RG8_UINT,
    RG8_SINT,
    RG8_UNORM,
    RG8_SNORM,
    RGB8_UINT,
    RGB8_SINT,
    RGB8_UNORM,
    RGB8_SNORM,
    R16_UINT,
    R16_SINT,
    R16_UNORM,
    R16_SNORM,
    R16_FLOAT,
    BGRA4_UNORM,
    B5G6R5_UNORM,
    B5G5R5A1_UNORM,
    RGBA8_UINT,
    RGBA8_SINT,
    RGBA8_UNORM,
    RGBA8_SNORM,
    BGRA8_UNORM,
    SRGBA8_UNORM,
    SBGRA8_UNORM,
    R10G10B10A2_UNORM,
    R11G11B10_FLOAT,
    RG16_UINT,
    RG16_SINT,
    RG16_UNORM,
    RG16_SNORM,
    RG16_FLOAT,
    RGB16_UINT,
    RGB16_SINT,
    RGB16_UNORM,
    RGB16_SNORM,
    RGB16_FLOAT,
    R32_UINT,
    R32_SINT,
    R32_FLOAT,
    RGBA16_UINT,
    RGBA16_SINT,
    RGBA16_FLOAT,
    RGBA16_UNORM,
    RGBA16_SNORM,
    RG32_UINT,
    RG32_SINT,
    RG32_FLOAT,
    RGB32_UINT,
    RGB32_SINT,
    RGB32_FLOAT,
    RGBA32_UINT,
    RGBA32_SINT,
    RGBA32_FLOAT,

    D16,
    D24S8,
    X24G8_UINT,
    D32,
    D32S8,
    X32G8_UINT,

    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UFLOAT,
    BC6H_SFLOAT,
    BC7_UNORM,
    BC7_UNORM_SRGB,

    COUNT,
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

enum class IndexType : uint8_t
{
    NONE,
    UINT16,
    UINT32,
};

enum class PrimitiveTopology : uint8_t
{
    TriangleList,
    TriangleStrip,
};

enum class CullMode : uint8_t
{
    None,
    Front,
    Back
};

enum class WindingMode : uint8_t
{
    CCW,
    CW
};

enum class PolygonMode : uint8_t
{
    Fill = 0,
    Line = 1,
};

enum class StencilOp : uint8_t
{
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap
};

enum class BlendOp : uint8_t
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class BlendFactor : uint8_t
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstColor,
    OneMinusDstColor,
    DstAlpha,
    OneMinusDstAlpha,
    SrcAlphaSaturated,
    BlendColor,
    OneMinusBlendColor,
    BlendAlpha,
    OneMinusBlendAlpha,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
};

enum class PipelineType : uint8_t
{
    Geometry,
    Mesh,
    Compute,
    RayTracing,
};

struct VertexInput
{
    struct VertexAttribute
    {
        uint32_t location = 0;
        uint32_t binding  = 0;
        Format   format   = Format::Undefined;
        uint32_t offset   = 0;

        bool operator==(const VertexAttribute& rhs) const
        {
            return location == rhs.location && binding == rhs.binding && format == rhs.format && offset == rhs.offset;
        }
    };

    struct VertexInputBinding
    {
        uint32_t stride = 0;
    };

    std::vector<VertexAttribute>    attributes;
    std::vector<VertexInputBinding> bindings;

    bool operator==(const VertexInput& rhs) const
    {
        if(attributes.size() != rhs.attributes.size() || bindings.size() != rhs.bindings.size())
        {
            return false;
        }

        for(auto idx = 0; idx < attributes.size(); ++idx)
        {
            if(rhs.attributes[idx] != attributes[idx])
            {
                return false;
            }
        }

        for(auto idx = 0; idx < attributes.size(); ++idx)
        {
            if(rhs.bindings[idx].stride != bindings[idx].stride)
            {
                return false;
            }
        }

        return true;
    }
};

template <typename T_Handle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
public:
    using HandleType     = T_Handle;
    using CreateInfoType = T_CreateInfo;

    ResourceHandle(HandleType handle, CreateInfoType createInfo = {}) : m_handle(handle), m_createInfo(createInfo)
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

}  // namespace aph::utils

#endif  // RESOURCE_H_
