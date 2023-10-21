#ifndef APH_SAMPLER_H
#define APH_SAMPLER_H

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

struct SamplerConvertInfo
{
    Format                        format = Format::Undefined;
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
    Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle, const YcbcrData* pYcbcr = nullptr);
    VkSamplerYcbcrConversion getConversion() const { return m_ycbcr.conversion; }
    bool                     isImmutable() const { return m_isImmutable; }
    bool                     hasConversion() const { return m_ycbcr.conversion != VK_NULL_HANDLE; }

private:
    Device* m_pDevice     = {};
    bool    m_isImmutable = {};

    YcbcrData m_ycbcr;
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
