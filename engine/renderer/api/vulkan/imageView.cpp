#include "imageView.h"
#include "device.h"
#include "image.h"
#include "vkUtils.h"

namespace vkl {

VulkanImageView* VulkanImageView::create(ImageViewCreateInfo *pCreateInfo, VulkanImage *pImage, VkImageView handle) {
    // Create a new ImageView class instance.
    VulkanImageView *imageView = new VulkanImageView;
    imageView->_device         = pImage->getDevice();
    imageView->_image          = pImage;
    imageView->_resourceHandle = handle;
    memcpy(&imageView->_createInfo, pCreateInfo, sizeof(ImageViewCreateInfo));

    return imageView;
}

VulkanImage *VulkanImageView::getImage() {
    return _image;
}

VulkanDevice *VulkanImageView::getDevice() {
    return _device;
}

void VulkanImageView::destroy() {
    vkDestroyImageView(_device->getLogicalDevice(), getHandle(), nullptr);
}
} // namespace vkl
