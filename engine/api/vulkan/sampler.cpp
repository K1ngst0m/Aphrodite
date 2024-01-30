#include "sampler.h"
#include "device.h"

namespace aph::vk
{
Sampler::Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle, const YcbcrData* pYcbcr) :
    ResourceHandle(handle, createInfo),
    m_pDevice(pDevice),
    m_isImmutable(createInfo.immutable)
{
    if(pYcbcr)
    {
        m_ycbcr.conversion = pYcbcr->conversion;
        m_ycbcr.info       = pYcbcr->info;
    }
}
}  // namespace aph::vk
