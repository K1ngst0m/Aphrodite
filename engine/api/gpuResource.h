#pragma once

#include "common/common.h"
#include "common/enum.h"

namespace aph
{
enum class BufferDomain : uint8_t
{
    Device, // Device local. Probably not visible from CPU.
    LinkedDeviceHost, // On desktop, directly mapped VRAM over PCI.
    Host, // Host-only, needs to be synced to GPU. Might be device local as well on iGPUs.
    CachedHost, //
};

enum class ImageDomain : uint8_t
{
    Device,
    Transient,
    LinearHostCached,
    LinearHost
};

using DeviceAddress = uint64_t;

enum class QueueType : uint8_t
{
    Unsupport = 0,
    Graphics,
    Compute,
    Transfer,
    Count,
};

enum class ShaderStage : uint8_t
{
    NA = 0,
    VS = 1,
    TCS = 2,
    TES = 3,
    GS = 4,
    FS = 5,
    CS = 6,
    TS = 7,
    MS = 8,
};
using ShaderStageFlags = Flags<ShaderStage>;

enum class ResourceState : uint32_t
{
    Undefined = 0,
    General = 0x00000001,
    UniformBuffer = 0x00000002,
    VertexBuffer = 0x00000004,
    IndexBuffer = 0x00000008,
    IndirectArgument = 0x00000010,
    ShaderResource = 0x00000020,
    UnorderedAccess = 0x00000040,
    RenderTarget = 0x00000080,
    DepthStencil = 0x00000100,
    StreamOut = 0x00000200,
    CopyDest = 0x00000400,
    CopySource = 0x00000800,
    ResolveDest = 0x00001000,
    ResolveSource = 0x00002000,
    Present = 0x00004000,
    AccelStructRead = 0x00008000,
    AccelStructWrite = 0x00010000,
};
using ResourceStateFlags = Flags<ResourceState>;

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
    uint32_t width = { 0 };
    uint32_t height = { 0 };
};

struct Extent3D
{
    uint32_t width = { 0 };
    uint32_t height = { 0 };
    uint32_t depth = { 0 };
};

struct Range
{
    std::size_t offset = { 0 };
    std::size_t size = { 0 };
};

struct DebugLabel
{
    std::string name;
    std::array<float, 4> color;
};

struct ShaderMacro
{
    std::string definition;
    std::string value;
};

struct ShaderConstant
{
    const void* pValue;
    uint32_t mIndex;
    uint32_t mSize;
};

enum class Filter
{
    Nearest,
    Linear,
    Cubic,
};

enum class SamplerAddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge,
};

enum class SamplerMipmapMode
{
    Nearest,
    Linear
};

enum class ImageType
{
    e1D,
    e2D,
    e3D
};

enum class ImageViewType
{
    e1D,
    e2D,
    e3D,
    Cube,
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

enum class WaveOpsSupport
{
    None = 0x0,
    Basic = 0x00000001,
    Vote = 0x00000002,
    Arithmetic = 0x00000004,
    Ballot = 0x00000008,
    Shuffle = 0x00000010,
    ShuffleRelative = 0x00000020,
    Clustered = 0x00000040,
    Quad = 0x00000080,
    All = 0x7FFFFFFF
};
using WaveOpsSupportFlags = Flags<WaveOpsSupport>;

constexpr uint32_t MAX_GPU_VENDOR_STRING_LENGTH = 256; // max size for GPUVendorPreset strings

struct GPUVendorPreset
{
    std::string vendorId;
    std::string modelId;
    std::string revisionId;
    std::string gpuName;
    std::string gpuDriverVersion;
    std::string gpuDriverDate;
    uint32_t rtCoresCount;
};

struct GPUFeature
{
    bool meshShading : 1 = false;
    bool multiDrawIndirect : 1 = false;
    bool tessellationSupported : 1 = false;
    bool samplerAnisotropy : 1 = false;
    bool rayTracing : 1 = false;
    bool bindless: 1 = false;
};

struct GPUProperties
{
    uint64_t vram;
    uint32_t uniformBufferAlignment;
    uint32_t uploadBufferTextureAlignment;
    uint32_t uploadBufferTextureRowAlignment;
    uint32_t maxVertexInputBindings;
    uint32_t maxRootSignatureDWORDS;
    uint32_t waveLaneCount;
    uint32_t maxBoundDescriptorSets;
    uint32_t timestampPeriod;
    WaveOpsSupportFlags waveOpsSupportFlags;
    GPUVendorPreset GpuVendorPreset;

    GPUFeature feature;
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
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList
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
    Undefined,
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
        uint32_t binding = 0;
        Format format = Format::Undefined;
        uint32_t offset = 0;
    };

    struct VertexInputBinding
    {
        uint32_t stride = 0;
    };

    std::vector<VertexAttribute> attributes;
    std::vector<VertexInputBinding> bindings;
};

struct ColorAttachment
{
    Format format = Format::Undefined;
    bool blendEnabled = false;
    BlendOp rgbBlendOp = BlendOp::Add;
    BlendOp alphaBlendOp = BlendOp::Add;
    BlendFactor srcRGBBlendFactor = BlendFactor::One;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstRGBBlendFactor = BlendFactor::Zero;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
};

struct DepthState
{
    bool enable = false;
    bool write = false;
    CompareOp compareOp = CompareOp::Always;
};

struct StencilState
{
    StencilOp stencilFailureOp = StencilOp::Keep;
    StencilOp depthFailureOp = StencilOp::Keep;
    StencilOp depthStencilPassOp = StencilOp::Keep;
    CompareOp stencilCompareOp = CompareOp::Always;
    uint32_t readMask = (uint32_t)~0;
    uint32_t writeMask = (uint32_t)~0;
};

struct DummyCreateInfo
{
    uint32_t typeId;
};

struct DummyHandle
{
    uint32_t typeId;
};

template <typename T_Handle = DummyHandle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
public:
    using HandleType = T_Handle;
    using CreateInfoType = T_CreateInfo;

    ResourceHandle(HandleType handle, CreateInfoType createInfo = {})
        : m_handle(handle)
        , m_createInfo(createInfo)
    {
        if constexpr (std::is_same_v<T_CreateInfo, DummyCreateInfo>)
        {
            m_createInfo.typeId = typeid(T_Handle).hash_code();
        }
        if constexpr (std::is_same_v<T_Handle, DummyHandle>)
        {
            m_handle.typeId = typeid(T_Handle).hash_code();
        }
    }
    operator T_Handle()
    {
        return m_handle;
    }
    operator T_Handle&()
    {
        return m_handle;
    }
    operator T_Handle&() const
    {
        return m_handle;
    }

    T_Handle& getHandle()
    {
        return m_handle;
    }
    const T_Handle& getHandle() const
    {
        return m_handle;
    }
    T_CreateInfo& getCreateInfo()
    {
        return m_createInfo;
    }
    const T_CreateInfo& getCreateInfo() const
    {
        return m_createInfo;
    }

    void setDebugName(const std::string& name)
    {
        m_debugName = name;
    }

    std::string_view getDebugName() const
    {
        return m_debugName;
    }

protected:
    T_Handle m_handle = {};
    T_CreateInfo m_createInfo = {};
    std::string m_debugName = {};
};

template <typename TObject>
concept ResourceHandleType = requires(TObject* obj, std::string name) {
    { obj->setDebugName(name) };
    { obj->getHandle() };
};
} // namespace aph
