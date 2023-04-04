#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_

#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;
class VulkanImageView;
class VulkanImage : public Image<VkImage>
{
public:
    VulkanImage(VulkanDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, VkDeviceMemory memory = VK_NULL_HANDLE);
    ~VulkanImage();

    VkDeviceMemory getMemory() { return m_memory; }
    VulkanDevice *getDevice() { return m_device; }

    VkResult bind(VkDeviceSize offset = 0) const;

    VulkanImageView *getImageView(Format imageFormat = FORMAT_UNDEFINED);

private:
    VulkanDevice *m_device = nullptr;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    std::unordered_map<uint32_t, VulkanImageView*> m_imageViewMap;

    void *m_mapped = nullptr;
};

}  // namespace vkl

#endif  // IMAGE_H_
