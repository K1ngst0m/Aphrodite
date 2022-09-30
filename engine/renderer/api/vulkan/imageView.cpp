#include "imageView.h"
#include "device.h"
#include "image.h"
#include "vkUtils.h"

namespace vkl {

VkResult VulkanImageView::create(VulkanImage *pImage, const void *pNext,
                                 VkImageViewType       viewType,
                                 VkFormat              format,
                                 VkComponentMapping    components,
                                 ImageSubresourceRange subresourceRange,
                                 VulkanImageView      *pView) {
    // Create a new Vulkan image view.
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext                 = pNext;
    createInfo.image                 = pImage->getHandle();
    createInfo.viewType              = viewType;
    createInfo.format                = format;
    memcpy(&createInfo.components, &components, sizeof(VkComponentMapping));
    createInfo.subresourceRange.aspectMask     = vkl::utils::getImageAspectFlags(format);
    createInfo.subresourceRange.baseMipLevel   = subresourceRange.baseMipLevel;
    createInfo.subresourceRange.levelCount     = subresourceRange.levelCount;
    createInfo.subresourceRange.baseArrayLayer = subresourceRange.baseArrayLayer;
    createInfo.subresourceRange.layerCount     = subresourceRange.layerCount;

    VkImageView handle = VK_NULL_HANDLE;
    auto        result = vkCreateImageView(pImage->getDevice()->getLogicalDevice(), &createInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    // Create a new ImageView class instance.
    VulkanImageView *imageView = new VulkanImageView;
    imageView->_device         = pImage->getDevice();
    imageView->_image          = pImage;
    imageView->_viewType       = static_cast<ImageViewType>(viewType);
    imageView->_format         = static_cast<Format>(format);
    memcpy(&imageView->_components, &components, sizeof(VkComponentMapping));
    memcpy(&imageView->_subresourceRange, &subresourceRange, sizeof(ImageSubresourceRange));
    imageView->resourceHandle = handle;

    // Copy object handle to parameter.
    pView = imageView;

    // Return success.
    return VK_SUCCESS;
}

VulkanImage *VulkanImageView::getImage() {
    return _image;
}

VulkanDevice *VulkanImageView::getDevice() {
    return _device;
}
} // namespace vkl
