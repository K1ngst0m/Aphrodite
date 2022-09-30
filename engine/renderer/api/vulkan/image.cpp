#include "image.h"
#include "device.h"

namespace vkl {

VkResult VulkanImage::bind(VkDeviceSize offset) const {
    return vkBindImageMemory(_device->getLogicalDevice(), resourceHandle, _memory, offset);
}
void VulkanImage::destroy() const {
    if (resourceHandle) {
        vkDestroyImage(_device->getLogicalDevice(), resourceHandle, nullptr);
    }
    if (_memory) {
        vkFreeMemory(_device->getLogicalDevice(), _memory, nullptr);
    }
}
VulkanImage *VulkanImage::createFromHandle(VulkanDevice *pDevice, ImageCreateInfo *pCreateInfo, VkImage image, VkDeviceMemory memory) {
    VulkanImage *instance = new VulkanImage;
    memcpy(&instance->createInfo, pCreateInfo, sizeof(ImageCreateInfo));
    instance->_device = pDevice;

    instance->resourceHandle = image;
    instance->_memory        = memory;

    return instance;
}

VkDeviceMemory VulkanImage::getMemory() {
    return _memory;
}
VulkanDevice *VulkanImage::getDevice() {
    return _device;
}
} // namespace vkl
