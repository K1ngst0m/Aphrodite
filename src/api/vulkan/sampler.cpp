#include "sampler.h"

namespace aph::vk
{
Sampler::Sampler(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
{
}

auto Sampler::getMinFilter() const -> Filter
{
    return getCreateInfo().minFilter;
}

auto Sampler::getMagFilter() const -> Filter
{
    return getCreateInfo().magFilter;
}

auto Sampler::getMipmapMode() const -> SamplerMipmapMode
{
    return getCreateInfo().mipMapMode;
}

auto Sampler::getAddressModeU() const -> SamplerAddressMode
{
    return getCreateInfo().addressU;
}

auto Sampler::getAddressModeV() const -> SamplerAddressMode
{
    return getCreateInfo().addressV;
}

auto Sampler::getAddressModeW() const -> SamplerAddressMode
{
    return getCreateInfo().addressW;
}

auto Sampler::getCompareOp() const -> CompareOp
{
    return getCreateInfo().compareFunc;
}

auto Sampler::getMipLodBias() const -> float
{
    return getCreateInfo().mipLodBias;
}

auto Sampler::getMinLod() const -> float
{
    return getCreateInfo().minLod;
}

auto Sampler::getMaxLod() const -> float
{
    return getCreateInfo().maxLod;
}

auto Sampler::getMaxAnisotropy() const -> float
{
    return getCreateInfo().maxAnisotropy;
}

auto Sampler::hasLodRange() const -> bool
{
    return getCreateInfo().setLodRange;
}

auto Sampler::matches(const SamplerCreateInfo& config) const -> bool
{
    return getCreateInfo() == config;
}

auto Sampler::isAnisotropic() const -> bool
{
    return getCreateInfo().maxAnisotropy > 0.0f;
}

auto Sampler::hasComparison() const -> bool
{
    return getCreateInfo().compareFunc != CompareOp::Never;
}

auto Sampler::isShadowSampler() const -> bool
{
    return hasComparison() && getCreateInfo().compareFunc == CompareOp::LessEqual;
}

auto SamplerCreateInfo::operator==(const SamplerCreateInfo& other) const -> bool
{
    return minFilter == other.minFilter && magFilter == other.magFilter && mipMapMode == other.mipMapMode &&
           addressU == other.addressU && addressV == other.addressV && addressW == other.addressW &&
           compareFunc == other.compareFunc && mipLodBias == other.mipLodBias && setLodRange == other.setLodRange &&
           (!setLodRange || (minLod == other.minLod && maxLod == other.maxLod)) && maxAnisotropy == other.maxAnisotropy;
}
auto SamplerCreateInfo::operator!=(const SamplerCreateInfo& other) const -> bool
{
    return !(*this == other);
}
} // namespace aph::vk
