#ifndef IMAGEVIEW_H_
#define IMAGEVIEW_H_

#include "renderer/gpuResource.h"

namespace vkl {
class VulkanDevice;
class VulkanImage;

class VulkanImageView : public ImageView<VkImageView> {
public:
    static VkResult create(VulkanImage *pImage, const void *pNext,
                           VkImageViewType       viewType,
                           VkFormat              format,
                           VkComponentMapping    components,
                           ImageSubresourceRange subresourceRange,
                           VulkanImageView      *pView);

    VulkanImage *getImage();
    VulkanDevice *getDevice();

private:
    VulkanDevice *_device;
    VulkanImage  *_image;
};
} // namespace vkl

#endif // IMAGEVIEW_H_
