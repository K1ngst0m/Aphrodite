#ifndef IMAGEVIEW_H_
#define IMAGEVIEW_H_

#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl {
class VulkanDevice;
class VulkanImage;

class VulkanImageView : public ImageView<VkImageView> {
public:
    static VulkanImageView* create(ImageViewCreateInfo* createInfo, VulkanImage *pImage, VkImageView handle);

    VulkanImage *getImage();
    VulkanDevice *getDevice();

    void destroy();

private:
    VulkanDevice *_device;
    VulkanImage  *_image;
};
} // namespace vkl

#endif // IMAGEVIEW_H_
