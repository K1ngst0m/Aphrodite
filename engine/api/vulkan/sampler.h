#pragma once

#include "api/gpuResource.h"
#include "allocator/objectPool.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

struct SamplerCreateInfo
{
    Filter             minFilter  = Filter::Linear;
    Filter             magFilter  = Filter::Linear;
    SamplerMipmapMode  mipMapMode = SamplerMipmapMode::Linear;
    SamplerAddressMode addressU   = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressV   = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressW   = SamplerAddressMode::ClampToEdge;

    CompareOp compareFunc   = CompareOp::Never;
    float     mipLodBias    = {};
    bool      setLodRange   = {};
    float     minLod        = {};
    float     maxLod        = {};
    float     maxAnisotropy = {};
    bool      immutable     = {};
};

class Sampler : public ResourceHandle<VkSampler, SamplerCreateInfo>
{
    friend class ObjectPool<Sampler>;

public:
    bool isImmutable() const { return m_isImmutable; }

private:
    Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle);

    Device* m_pDevice     = {};
    bool    m_isImmutable = {};
};

}  // namespace aph::vk

namespace aph::vk::init
{
inline SamplerCreateInfo samplerCreateInfo(SamplerPreset preset)
{
    SamplerCreateInfo ci;
    ci.maxLod        = ::vk::LodClampNone;
    ci.maxAnisotropy = 1.0f;

    switch(preset)
    {
    case SamplerPreset::NearestShadow:
    case SamplerPreset::LinearShadow:
        ci.compareFunc = CompareOp::LessEqual;
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
        ci.mipMapMode = SamplerMipmapMode::Linear;
        break;
    default:
        ci.mipMapMode = SamplerMipmapMode::Nearest;
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
        ci.magFilter = Filter::Linear;
        ci.minFilter = Filter::Linear;
        break;
    default:
        ci.magFilter = Filter::Nearest;
        ci.minFilter = Filter::Nearest;
        break;
    }

    switch(preset)
    {
    default:
    case SamplerPreset::DefaultGeometryFilterWrap:
    case SamplerPreset::LinearWrap:
    case SamplerPreset::NearestWrap:
    case SamplerPreset::TrilinearWrap:
        ci.addressU = SamplerAddressMode::Repeat;
        ci.addressV = SamplerAddressMode::Repeat;
        ci.addressW = SamplerAddressMode::Repeat;
        break;

    case SamplerPreset::DefaultGeometryFilterClamp:
    case SamplerPreset::LinearClamp:
    case SamplerPreset::NearestClamp:
    case SamplerPreset::TrilinearClamp:
    case SamplerPreset::NearestShadow:
    case SamplerPreset::LinearShadow:
        ci.addressU = SamplerAddressMode::ClampToEdge;
        ci.addressV = SamplerAddressMode::ClampToEdge;
        ci.addressW = SamplerAddressMode::ClampToEdge;
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
