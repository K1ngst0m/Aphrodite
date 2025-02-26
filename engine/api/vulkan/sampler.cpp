#include "sampler.h"
#include "device.h"

namespace aph::vk
{
Sampler::Sampler(Device* pDevice, const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_pDevice(pDevice),
    m_isImmutable(createInfo.immutable)
{
}
}  // namespace aph::vk
