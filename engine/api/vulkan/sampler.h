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
    Sampler(Device* pDevice, const VkSamplerCreateInfo& createInfo, VkSampler handle, bool immutable) :
        m_pDevice(pDevice),
        m_isImmutable(immutable)
    {
        getHandle()     = handle;
        getCreateInfo() = createInfo;
    }

private:
    Device* m_pDevice{};
    bool    m_isImmutable{};
};

class ImmutableYcbcrConversion
{
public:
    ImmutableYcbcrConversion(Device* device, const VkSamplerYcbcrConversionCreateInfo& info);
    ~ImmutableYcbcrConversion();
    void operator=(const ImmutableYcbcrConversion&)           = delete;
    ImmutableYcbcrConversion(const ImmutableYcbcrConversion&) = delete;

    VkSamplerYcbcrConversion get_conversion() const { return conversion; }

private:
    Device*                  device{};
    VkSamplerYcbcrConversion conversion{};
};

class ImmutableSampler
{
public:
    ImmutableSampler(Device* device, const VkSamplerCreateInfo& info, const ImmutableYcbcrConversion* ycbcr);
    void operator=(const ImmutableSampler&)   = delete;
    ImmutableSampler(const ImmutableSampler&) = delete;

    const Sampler& get_sampler() const { return *sampler; }

    VkSamplerYcbcrConversion get_ycbcr_conversion() const { return ycbcr ? ycbcr->get_conversion() : VK_NULL_HANDLE; }

private:
    Device*                         device{};
    const ImmutableYcbcrConversion* ycbcr{};
    Sampler*                        sampler{};
};

}  // namespace aph::vk

#endif
