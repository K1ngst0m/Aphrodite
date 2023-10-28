#ifndef APH_SAMPLER_H
#define APH_SAMPLER_H

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
    friend class ObjectPool<Sampler>;
public:
    VkSamplerYcbcrConversion getConversion() const { return m_ycbcr.conversion; }
    bool                     isImmutable() const { return m_isImmutable; }
    bool                     hasConversion() const { return m_ycbcr.conversion != VK_NULL_HANDLE; }

private:
    Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle, const YcbcrData* pYcbcr = nullptr);

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
    ci.maxLod        = VK_LOD_CLAMP_NONE;
    ci.maxAnisotropy = 1.0f;

    switch(preset)
    {
    case SamplerPreset::NearestShadow:
    case SamplerPreset::LinearShadow:
        ci.compareFunc = VK_COMPARE_OP_LESS_OR_EQUAL;
        break;
    default:
        break;
    }

    switch(preset)
    {
    case SamplerPreset::TrilinearClamp:
    case SamplerPreset::TrilinearWrap:
    case SamplerPreset::DefaultGeometryFilterWrap:
    case SamplerPreset::DefaultGeometryFilterClamp:
        ci.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;

    default:
        ci.mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        break;
    }

    switch(preset)
    {
    case SamplerPreset::DefaultGeometryFilterClamp:
    case SamplerPreset::DefaultGeometryFilterWrap:
    case SamplerPreset::LinearClamp:
    case SamplerPreset::LinearWrap:
    case SamplerPreset::TrilinearClamp:
    case SamplerPreset::TrilinearWrap:
    case SamplerPreset::LinearShadow:
        ci.magFilter = VK_FILTER_LINEAR;
        ci.minFilter = VK_FILTER_LINEAR;
        break;

    default:
        ci.magFilter = VK_FILTER_NEAREST;
        ci.minFilter = VK_FILTER_NEAREST;
        break;
    }

    switch(preset)
    {
    default:
    case SamplerPreset::DefaultGeometryFilterWrap:
    case SamplerPreset::LinearWrap:
    case SamplerPreset::NearestWrap:
    case SamplerPreset::TrilinearWrap:
        ci.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ci.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ci.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;

    case SamplerPreset::DefaultGeometryFilterClamp:
    case SamplerPreset::LinearClamp:
    case SamplerPreset::NearestClamp:
    case SamplerPreset::TrilinearClamp:
    case SamplerPreset::NearestShadow:
    case SamplerPreset::LinearShadow:
        ci.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    }

    switch(preset)
    {
    case SamplerPreset::DefaultGeometryFilterWrap:
    case SamplerPreset::DefaultGeometryFilterClamp:
        // TODO limited by device properties
        ci.maxAnisotropy = 16.0f;
        break;

    default:
        break;
    }

    return ci;
}
}  // namespace aph::vk::init

#endif
