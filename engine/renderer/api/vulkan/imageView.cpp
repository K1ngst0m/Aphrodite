#include "imageView.h"
#include "device.h"
#include "image.h"
#include "vkUtils.h"

namespace vkl {

VulkanImageView::VulkanImageView(const ImageViewCreateInfo &createInfo, VulkanImage *pImage, VkImageView handle)
    : _device(pImage->getDevice()), _image(pImage)
{
    getHandle() = handle;
    getCreateInfo() = createInfo;
}

} // namespace vkl
