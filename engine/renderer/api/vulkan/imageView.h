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

    VulkanImage *getImage() { return _image; }
    VulkanDevice *getDevice() { return _device; }

private:
    VulkanDevice *_device;
    VulkanImage *_image;
};
}  // namespace vkl

#endif  // IMAGEVIEW_H_
