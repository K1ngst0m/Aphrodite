#ifndef APH_SAMPLER_H
#define APH_SAMPLER_H

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

struct SamplerConvertInfo
{
    VkFormat                      format = VK_FORMAT_UNDEFINED;
    VkSamplerYcbcrModelConversion model;
    VkSamplerYcbcrRange           range;
    VkChromaLocation              chromaOffsetX;
    VkChromaLocation              chromaOffsetY;
    VkFilter                      chromaFilter;
    bool                          forceExplicitReconstruction;
};

struct SamplerCreateInfo
{
    VkFilter             minFilter     = VK_FILTER_LINEAR;
    VkFilter             magFilter     = VK_FILTER_LINEAR;
    VkSamplerMipmapMode  mipMapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressU      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode addressV      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode addressW      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkCompareOp          compareFunc   = VK_COMPARE_OP_NEVER;
    float                mipLodBias    = {};
    bool                 setLodRange   = {};
    float                minLod        = {};
    float                maxLod        = {};
    float                maxAnisotropy = {};
    bool                 immutable     = {};

    SamplerConvertInfo* pConvertInfo = {};
};

struct YcbcrData
{
    VkSamplerYcbcrConversion     conversion;
    VkSamplerYcbcrConversionInfo info;
};

class Sampler : public ResourceHandle<VkSampler, SamplerCreateInfo>
{
public:
    Sampler(Device* pDevice, const SamplerCreateInfo& createInfo, VkSampler handle, const YcbcrData* pYcbcr = nullptr);

private:
    Device* m_pDevice     = {};
    bool    m_isImmutable = {};

    YcbcrData m_ycbcr;
};

class ImmutableYcbcrConversion
{
public:
    // ImmutableYcbcrConversion(Device* device, const VkSamplerYcbcrConversionCreateInfo& info);
    ~ImmutableYcbcrConversion();
    void operator=(const ImmutableYcbcrConversion&)           = delete;
    ImmutableYcbcrConversion(const ImmutableYcbcrConversion&) = delete;

    VkSamplerYcbcrConversion getConversion() const { return m_conversion; }

private:
    Device*                  m_pDevice    = {};
    VkSamplerYcbcrConversion m_conversion = {};
};

class ImmutableSampler
{
public:
    ImmutableSampler(Device* device, const SamplerCreateInfo& info, const ImmutableYcbcrConversion* ycbcr = nullptr);
    void operator=(const ImmutableSampler&)   = delete;
    ImmutableSampler(const ImmutableSampler&) = delete;

    Sampler* getSampler() const { return m_pSampler; }

    VkSamplerYcbcrConversion getYcbcrConversion() const;

private:
    Device*                         m_pDevice  = {};
    const ImmutableYcbcrConversion* m_pYcbcr   = {};
    Sampler*                        m_pSampler = {};
};

}  // namespace aph::vk

namespace aph::vk::init
{
inline SamplerCreateInfo samplerCreateInfo2(SamplerPreset preset)
{
    SamplerCreateInfo ci;
    ci.magFilter     = VK_FILTER_LINEAR;
    ci.minFilter     = VK_FILTER_LINEAR;
    ci.addressU      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressV      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressW      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.maxAnisotropy = 1.0f;
    ci.mipMapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.mipLodBias    = 0.0f;
    ci.minLod        = 0.0f;
    ci.maxLod        = 1.0f;

    switch(preset)
    {
    case SamplerPreset::Nearest:
        ci.magFilter = VK_FILTER_NEAREST;
        ci.minFilter = VK_FILTER_NEAREST;
        break;
    case SamplerPreset::Linear:
        ci.magFilter = VK_FILTER_LINEAR;
        ci.minFilter = VK_FILTER_LINEAR;
        break;
    case SamplerPreset::Anisotropic:
        ci.maxAnisotropy = 16.0f;
        break;
    case SamplerPreset::Mipmap:
        ci.minFilter  = VK_FILTER_LINEAR;
        ci.magFilter  = VK_FILTER_LINEAR;
        ci.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        ci.minLod     = 0.0f;
        ci.maxLod     = 8.0f;
        break;
    }

    return ci;
}
}  // namespace aph::vk::init

#endif
