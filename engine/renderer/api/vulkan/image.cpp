#include "image.h"
#include "device.h"

namespace vkl {

VulkanImage::VulkanImage(VulkanDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, VkDeviceMemory memory)
    : _device(pDevice), _memory(memory)
{
    _handle = image;
    _createInfo = createInfo;
}

VkResult VulkanImage::bind(VkDeviceSize offset) const
{
    return vkBindImageMemory(_device->getHandle(), _handle, _memory, offset);
}
}  // namespace vkl
