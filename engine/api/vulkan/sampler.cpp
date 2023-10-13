#include "sampler.h"
#include "device.h"

namespace aph::vk
{
ImmutableSampler::ImmutableSampler(Device* device, const SamplerCreateInfo& createInfo,
                                   const ImmutableYcbcrConversion* ycbcr) :
    m_pDevice(device),
    m_pYcbcr(ycbcr)
{
    VkSamplerYcbcrConversionInfo convInfo = {VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};

    auto info = createInfo;
    if(ycbcr)
    {
        // TODO
        // APH_ASSERT(false);
        // convInfo.conversion = ycbcr->getConversion();
        // info.pNext          = &convInfo;
    }

#ifdef APH_DEBUG
    VK_LOG_INFO("Creating immutable sampler.\n");
#endif

    info.immutable = true;
    if(m_pDevice->create(info, &m_pSampler) != VK_SUCCESS)
    {
        VK_LOG_ERR("Failed to create sampler.\n");
    }
}
VkSamplerYcbcrConversion aph::vk::ImmutableSampler::getYcbcrConversion() const
{
    // TODO
    APH_ASSERT(false);
    return m_pYcbcr ? m_pYcbcr->getConversion() : VK_NULL_HANDLE;
}
Sampler::Sampler(Device* pDevice, const SamplerCreateInfo& createInfo, VkSampler handle, const YcbcrData* pYcbcr) :
    m_pDevice(pDevice),
    m_isImmutable(createInfo.immutable)
{
    getHandle()     = handle;
    getCreateInfo() = createInfo;

    if(pYcbcr)
    {
        m_ycbcr.conversion = pYcbcr->conversion;
        m_ycbcr.info       = pYcbcr->info;
    }
}
}  // namespace aph::vk
