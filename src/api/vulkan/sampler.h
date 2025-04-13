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

    auto operator==(const SamplerCreateInfo& other) const -> bool;
    auto operator!=(const SamplerCreateInfo& other) const -> bool;
};

class Sampler : public ResourceHandle<::vk::Sampler, SamplerCreateInfo>
{
    friend class ThreadSafeObjectPool<Sampler>;

public:
    auto getMinFilter() const -> Filter;
    auto getMagFilter() const -> Filter;
    auto getMipmapMode() const -> SamplerMipmapMode;
    auto getAddressModeU() const -> SamplerAddressMode;
    auto getAddressModeV() const -> SamplerAddressMode;
    auto getAddressModeW() const -> SamplerAddressMode;
    auto getCompareOp() const -> CompareOp;
    auto getMipLodBias() const -> float;
    auto getMinLod() const -> float;
    auto getMaxLod() const -> float;
    auto getMaxAnisotropy() const -> float;
    auto hasLodRange() const -> bool;

    auto matches(const SamplerCreateInfo& config) const -> bool;
    auto isAnisotropic() const -> bool;
    auto hasComparison() const -> bool;
    auto isShadowSampler() const -> bool;

private:
    Sampler(const CreateInfoType& createInfo, HandleType handle);
};

} // namespace aph::vk
