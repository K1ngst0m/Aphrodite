#include "image.h"
#include "device.h"

namespace vkl {

VkResult VulkanImage::bind(VkDeviceSize offset) const {
    return vkBindImageMemory(_device->getHandle(), _handle, _memory, offset);
}
VulkanImage *VulkanImage::CreateFromHandle(VulkanDevice *pDevice, ImageCreateInfo *pCreateInfo, VkImage image, VkDeviceMemory memory) {
    assert(image != VK_NULL_HANDLE);

    VulkanImage *instance = new VulkanImage;
    memcpy(&instance->_createInfo, pCreateInfo, sizeof(ImageCreateInfo));
    instance->_device             = pDevice;
    instance->_handle             = image;
    instance->_memory             = memory;

    return instance;
}

} // namespace vkl
