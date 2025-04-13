#include "sampler.h"

namespace aph::vk
{
Sampler::Sampler(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
{
}

} // namespace aph::vk
