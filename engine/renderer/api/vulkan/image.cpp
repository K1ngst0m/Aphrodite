#include "image.h"
#include "device.h"

namespace vkl {

VkResult VulkanImage::bind(VkDeviceSize offset) const {
    return vkBindImageMemory(_device->getLogicalDevice(), _resourceHandle, _memory, offset);
}
void VulkanImage::destroy() const {
    if (_resourceHandle) {
        vkDestroyImage(_device->getLogicalDevice(), _resourceHandle, nullptr);
    }
    if (_memory) {
        vkFreeMemory(_device->getLogicalDevice(), _memory, nullptr);
    }
}
VulkanImage *VulkanImage::createFromHandle(VulkanDevice *pDevice, ImageCreateInfo *pCreateInfo, VkImageLayout defaultLayout, VkImage image, VkDeviceMemory memory) {
    VulkanImage *instance = new VulkanImage;
    memcpy(&instance->createInfo, pCreateInfo, sizeof(ImageCreateInfo));
    instance->_device              = pDevice;
    instance->_defaultImageLayout = defaultLayout;
    instance->_resourceHandle       = image;
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
