#ifndef IMAGEVIEW_H_
#define IMAGEVIEW_H_

#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;
class VulkanImage;

class VulkanImageView : public ImageView<VkImageView>
{
public:
    VulkanImageView(const ImageViewCreateInfo &createInfo, VulkanImage *pImage, VkImageView handle);

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

#endif  // IMAGEVIEW_H_
