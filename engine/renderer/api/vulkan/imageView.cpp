#include "imageView.h"
#include "device.h"
#include "image.h"
#include "vkUtils.h"

namespace vkl {

VulkanImageView *VulkanImageView::createFromHandle(ImageViewCreateInfo *pCreateInfo, VulkanImage *pImage, VkImageView handle) {
    // Create a new ImageView class instance.
    VulkanImageView *imageView = new VulkanImageView;
    imageView->_device         = pImage->getDevice();
    imageView->_image          = pImage;
    imageView->_handle         = handle;
    memcpy(&imageView->_createInfo, pCreateInfo, sizeof(ImageViewCreateInfo));

    return imageView;
}

} // namespace vkl
