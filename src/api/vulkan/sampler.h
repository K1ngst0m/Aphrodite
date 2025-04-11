#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct SamplerCreateInfo
{
    Filter minFilter             = Filter::Linear;
    Filter magFilter             = Filter::Linear;
    SamplerMipmapMode mipMapMode = SamplerMipmapMode::Linear;
    SamplerAddressMode addressU  = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressV  = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressW  = SamplerAddressMode::ClampToEdge;

    CompareOp compareFunc = CompareOp::Never;
    float mipLodBias      = {};
    bool setLodRange      = {};
    float minLod          = {};
    float maxLod          = {};
    float maxAnisotropy   = {};

    // Compare two sampler configurations for equality
    bool operator==(const SamplerCreateInfo& other) const
    {
        return minFilter == other.minFilter && magFilter == other.magFilter && mipMapMode == other.mipMapMode &&
               addressU == other.addressU && addressV == other.addressV && addressW == other.addressW &&
               compareFunc == other.compareFunc && mipLodBias == other.mipLodBias && setLodRange == other.setLodRange &&
               (!setLodRange || (minLod == other.minLod && maxLod == other.maxLod)) &&
               maxAnisotropy == other.maxAnisotropy;
    }

    // Compare for inequality
    bool operator!=(const SamplerCreateInfo& other) const
    {
        return !(*this == other);
    }
};

class Sampler : public ResourceHandle<::vk::Sampler, SamplerCreateInfo>
{
    friend class ThreadSafeObjectPool<Sampler>;

public:
    Filter getMinFilter() const
    {
        return getCreateInfo().minFilter;
    }
    Filter getMagFilter() const
    {
        return getCreateInfo().magFilter;
    }
    SamplerMipmapMode getMipmapMode() const
    {
        return getCreateInfo().mipMapMode;
    }
    SamplerAddressMode getAddressModeU() const
    {
        return getCreateInfo().addressU;
    }
    SamplerAddressMode getAddressModeV() const
    {
        return getCreateInfo().addressV;
    }
    SamplerAddressMode getAddressModeW() const
    {
        return getCreateInfo().addressW;
    }
    CompareOp getCompareOp() const
    {
        return getCreateInfo().compareFunc;
    }
    float getMipLodBias() const
    {
        return getCreateInfo().mipLodBias;
    }
    float getMinLod() const
    {
        return getCreateInfo().minLod;
    }
    float getMaxLod() const
    {
        return getCreateInfo().maxLod;
    }
    float getMaxAnisotropy() const
    {
        return getCreateInfo().maxAnisotropy;
    }
    bool hasLodRange() const
    {
        return getCreateInfo().setLodRange;
    }

    bool matches(const SamplerCreateInfo& config) const
    {
        return getCreateInfo() == config;
    }
    bool isAnisotropic() const
    {
        return getCreateInfo().maxAnisotropy > 0.0f;
    }
    bool hasComparison() const
    {
        return getCreateInfo().compareFunc != CompareOp::Never;
    }
    bool isShadowSampler() const
    {
        return hasComparison() && getCreateInfo().compareFunc == CompareOp::LessEqual;
    }

private:
    Sampler(const CreateInfoType& createInfo, HandleType handle);
};

} // namespace aph::vk
