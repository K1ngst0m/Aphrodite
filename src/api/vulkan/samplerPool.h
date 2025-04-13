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
    auto operator=(const SamplerPool&) -> SamplerPool& = delete;
    auto operator=(SamplerPool&&) -> SamplerPool&      = delete;
    explicit SamplerPool(Device* pDevice);
    ~SamplerPool();

    auto initialize() -> Result;
    auto getSampler(PresetSamplerType type) const -> Sampler*;
    auto findMatchingSampler(const SamplerCreateInfo& config) const -> Sampler*;
    static auto getCreateInfoFromType(PresetSamplerType type) -> SamplerCreateInfo;

private:
    auto createPredefinedSampler(PresetSamplerType type) -> Result;

private:
    Device* m_pDevice;
    std::array<Sampler*, kSamplerTypeCount> m_samplers{};
    mutable std::mutex m_mutex;
};

} // namespace aph::vk
