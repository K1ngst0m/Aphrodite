#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

struct SamplerCreateInfo
{
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    SamplerMipmapMode mipMapMode = SamplerMipmapMode::Linear;
    SamplerAddressMode addressU = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressV = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressW = SamplerAddressMode::ClampToEdge;

    CompareOp compareFunc = CompareOp::Never;
    float mipLodBias = {};
    bool setLodRange = {};
    float minLod = {};
    float maxLod = {};
    float maxAnisotropy = {};
    bool immutable = {};

    SamplerCreateInfo& preset(SamplerPreset preset);
};

class Sampler : public ResourceHandle<VkSampler, SamplerCreateInfo>
{
    friend class ObjectPool<Sampler>;

public:
    bool isImmutable() const
    {
        return m_isImmutable;
    }

private:
    Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle);

    Device* m_pDevice = {};
    bool m_isImmutable = {};
};

} // namespace aph::vk
