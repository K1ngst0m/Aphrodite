#pragma once

#include "forward.h"
#include "api/gpuResource.h"
#include "common/enum.h"
#include "common/hash.h"
#include "common/smallVector.h"

namespace aph
{
enum class MaterialDomain : uint8_t
{
    eOpaque = 0,
    eTranslucent,
    eTransmissive,
    eSubsurface,
    eHair,
    ePostProcess,
    eCompute
};

enum class MaterialFeatureBits : uint32_t
{
    eNone        = 0,
    eAlbedo      = 1 << 0,
    eMetallic    = 1 << 1,
    eRoughness   = 1 << 2,
    eNormal      = 1 << 3,
    eEmissive    = 1 << 4,
    eAO          = 1 << 5,
    eTranslucent = 1 << 6,
    eRefraction  = 1 << 7,
    eAnisotropic = 1 << 8,
    eSubsurface  = 1 << 9,
    eClearCoat   = 1 << 10,
    eRayTraced   = 1 << 11,
    eVolumetric  = 1 << 12,
    eCustomData  = 1 << 13,
    eStandard    = eAlbedo | eMetallic | eRoughness | eNormal,
    eAdvanced    = eStandard | eEmissive | eAO,
    eAll         = 0xFFFFFFFF
};
using MaterialFeatureFlags = Flags<MaterialFeatureBits>;

template <>
struct FlagTraits<MaterialFeatureBits>
{
    static constexpr bool isBitmask                = true;
    static constexpr MaterialFeatureFlags allFlags = MaterialFeatureBits::eAll;
};

enum class DataType : uint8_t
{
    // Scalar types
    eFloat = 0,
    eInt,
    eUint,
    eBool,

    // Vector types
    eVec2,
    eVec3,
    eVec4,
    eIvec2,
    eIvec3,
    eIvec4,
    eUvec2,
    eUvec3,
    eUvec4,

    // Matrix types
    eMat2,
    eMat3,
    eMat4,

    // Texture types
    eTexture2D,
    eTextureCube,
    eTexture2DArray,
    eTexture3D,

    // Special types
    eSampler,
    eBuffer
};

// Parameter descriptor for material template parameters
struct MaterialParameterDesc
{
    std::string_view name;
    DataType type;
    uint32_t offset;
    uint32_t size;
    bool isTexture;
};

class MaterialTemplate
{
public:
    // Constructor
    MaterialTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags);
    ~MaterialTemplate();

    // Getters
    [[nodiscard]] auto getName() const -> std::string_view;
    [[nodiscard]] auto getDomain() const -> MaterialDomain;
    [[nodiscard]] auto getFeatureFlags() const -> MaterialFeatureFlags;
    
    // Parameter management
    void addParameter(const MaterialParameterDesc& parameter);
    [[nodiscard]] auto getParameterLayout() const -> const ParameterLayout&;

    // Shader code management
    void setShaderCode(ShaderStage stage, std::string_view code);
    [[nodiscard]] auto getShaderCode(ShaderStage stage) const -> std::string_view;

private:
    std::string m_name;
    MaterialDomain m_domain{ MaterialDomain::eOpaque };
    MaterialFeatureFlags m_featureFlags{ MaterialFeatureBits::eNone };
    ParameterLayout* m_parameterLayout{ nullptr };
    HashMap<ShaderStage, std::string> m_shaderCode;
};
} // namespace aph
