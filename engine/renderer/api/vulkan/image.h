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

class VulkanImageView : public ImageView<VkImageView>
{
public:
    VulkanImageView(const ImageViewCreateInfo &createInfo, VulkanImage *pImage, VkImageView handle)
        : m_device(pImage->getDevice()), m_image(pImage)
    {
        getHandle() = handle;
        getCreateInfo() = createInfo;
    }

    VulkanImage *getImage() { return m_image; }
    VulkanDevice *getDevice() { return m_device; }

    VkDescriptorImageInfo & getDescInfoMap(VkImageLayout layout){
        if(!m_descInfoMap.count(layout))
        {
            m_descInfoMap[layout] = {
                .sampler = VK_NULL_HANDLE,
                .imageView = getHandle(),
                .imageLayout = layout,
            };
        }
        return m_descInfoMap[layout];
    }

private:
    VulkanDevice *m_device;
    VulkanImage *m_image;
    std::unordered_map<VkImageLayout, VkDescriptorImageInfo> m_descInfoMap;
};

}  // namespace vkl

#endif  // IMAGE_H_
