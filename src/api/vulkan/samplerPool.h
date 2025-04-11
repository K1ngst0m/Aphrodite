#pragma once

#include "forward.h"
#include "sampler.h"

namespace aph::vk
{
enum class PresetSamplerType : uint8_t
{
    // Standard texture samplers
    eLinearClampMipmap,
    eLinearWrapMipmap,
    eLinearMirrorMipmap,
    eNearestClampMipmap,
    eNearestWrapMipmap,

    // Anisotropic samplers
    eAnisotropicClamp,
    eAnisotropicWrap,

    // Special purpose samplers
    eShadowPcf,
    eShadowEsm,
    eCubemap,
    eCubemapLow,
    ePointClamp,

    eCount
};

constexpr size_t kSamplerTypeCount = static_cast<size_t>(PresetSamplerType::eCount);

class SamplerPool
{
public:
    SamplerPool(const SamplerPool&)            = delete;
    SamplerPool(SamplerPool&&)                 = delete;
    SamplerPool& operator=(const SamplerPool&) = delete;
    SamplerPool& operator=(SamplerPool&&)      = delete;
    explicit SamplerPool(Device* pDevice);
    ~SamplerPool();

    Result initialize();
    Sampler* getSampler(PresetSamplerType type) const;
    Sampler* findMatchingSampler(const SamplerCreateInfo& config) const;
    static SamplerCreateInfo getCreateInfoFromType(PresetSamplerType type);

private:
    Result createPredefinedSampler(PresetSamplerType type);

private:
    Device* m_pDevice;
    std::array<Sampler*, kSamplerTypeCount> m_samplers{};
    mutable std::mutex m_mutex;
};

} // namespace aph::vk
