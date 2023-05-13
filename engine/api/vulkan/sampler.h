#ifndef APH_SAMPLER_H
#define APH_SAMPLER_H

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;

class Sampler : public ResourceHandle<VkSampler, VkSamplerCreateInfo>
{
public:
    Sampler(Device* pDevice, const VkSamplerCreateInfo& createInfo, VkSampler handle, bool immutable);

private:
    Device* m_pDevice     = {};
    bool    m_isImmutable = {};
};

class ImmutableYcbcrConversion
{
public:
    ImmutableYcbcrConversion(Device* device, const VkSamplerYcbcrConversionCreateInfo& info);
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
    ImmutableSampler(Device* device, const VkSamplerCreateInfo& info, const ImmutableYcbcrConversion* ycbcr = nullptr);
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

#endif
