#include "image.h"
#include "device.h"

namespace vkl {

VulkanImage::VulkanImage(VulkanDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, VkDeviceMemory memory)
    : _device(pDevice), _memory(memory)
{
    getHandle() = image;
    getCreateInfo() = createInfo;
}

VkResult VulkanImage::bind(VkDeviceSize offset) const
{
    return vkBindImageMemory(_device->getHandle(), getHandle(), _memory, offset);
}
}  // namespace vkl
