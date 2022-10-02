#include "image.h"
#include "device.h"

namespace vkl {

VkResult VulkanImage::bind(VkDeviceSize offset) const {
    return vkBindImageMemory(_device->getLogicalDevice(), _handle, _memory, offset);
}
VulkanImage *VulkanImage::createFromHandle(VulkanDevice *pDevice, ImageCreateInfo *pCreateInfo, VkImageLayout defaultLayout, VkImage image, VkDeviceMemory memory) {
    VulkanImage *instance = new VulkanImage;
    memcpy(&instance->_createInfo, pCreateInfo, sizeof(ImageCreateInfo));
    instance->_device              = pDevice;
    instance->_defaultImageLayout = defaultLayout;
    instance->_handle       = image;
    instance->_memory              = memory;

    return instance;
}

VkDeviceMemory VulkanImage::getMemory() {
    return _memory;
}
VulkanDevice *VulkanImage::getDevice() {
    return _device;
}
VkImageLayout VulkanImage::getImageLayout() {
    return _defaultImageLayout;
}
} // namespace vkl
