#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_

#include "device.h"

namespace vkl
{
class VulkanImage : public Image<VkImage>
{
public:
    static VulkanImage *CreateFromHandle(VulkanDevice *pDevice, ImageCreateInfo *pCreateInfo,
                                         VkImage image, VkDeviceMemory memory = VK_NULL_HANDLE);

    VkDeviceMemory getMemory() { return _memory; }
    VulkanDevice *getDevice() { return _device; }

    VkResult bind(VkDeviceSize offset = 0) const;

private:
    VulkanDevice *_device = nullptr;
    VkDeviceMemory _memory = VK_NULL_HANDLE;

    void *_mapped = nullptr;
};

}  // namespace vkl

#endif  // IMAGE_H_
