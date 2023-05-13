#include "sampler.h"
#include "device.h"

using namespace aph::vk;

ImmutableSampler::ImmutableSampler(Device* device, const VkSamplerCreateInfo& createInfo,
                                   const ImmutableYcbcrConversion* ycbcr) :
    m_pDevice(device),
    m_pYcbcr(ycbcr)
{
    VkSamplerYcbcrConversionInfo convInfo = {VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};

    auto info = createInfo;
    if(ycbcr)
    {
        // TODO
        APH_ASSERT(false);
        convInfo.conversion = ycbcr->getConversion();
        info.pNext          = &convInfo;
    }

#ifdef APH_DEBUG
    VK_LOG_INFO("Creating immutable sampler.\n");
#endif

    if(m_pDevice->createSampler(info, &m_pSampler, true) != VK_SUCCESS)
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
aph::vk::Sampler::Sampler(Device* pDevice, const VkSamplerCreateInfo& createInfo, VkSampler handle, bool immutable) :
    m_pDevice(pDevice),
    m_isImmutable(immutable)
{
    getHandle()     = handle;
    getCreateInfo() = createInfo;
}
