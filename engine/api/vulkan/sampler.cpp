#include "sampler.h"
#include "device.h"

namespace aph::vk
{
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
