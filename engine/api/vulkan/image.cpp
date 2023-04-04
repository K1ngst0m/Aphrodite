#include "image.h"
#include "device.h"

namespace vkl {

VulkanImage::VulkanImage(VulkanDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, VkDeviceMemory memory)
    : m_device(pDevice), m_memory(memory)
{
    getHandle() = image;
    getCreateInfo() = createInfo;
}

VulkanImage::~VulkanImage(){
    for (auto &[_, imageView] : m_imageViewMap){
        m_device->destroyImageView(imageView);
    }
}

VkResult VulkanImage::bind(VkDeviceSize offset) const
{
    return vkBindImageMemory(m_device->getHandle(), getHandle(), m_memory, offset);
}
VulkanImageView *VulkanImage::getImageView(Format imageFormat)
{
    if (imageFormat == FORMAT_UNDEFINED)
    {
        imageFormat = m_createInfo.format;
    }

    if(!m_imageViewMap.count(imageFormat))
    {
        std::unordered_map<ImageType, ImageViewType> imageTypeMap{
            {IMAGE_TYPE_1D, IMAGE_VIEW_TYPE_1D},
            {IMAGE_TYPE_2D, IMAGE_VIEW_TYPE_2D},
            {IMAGE_TYPE_2D, IMAGE_VIEW_TYPE_3D},
        };
        ImageViewCreateInfo createInfo{
            .viewType = imageTypeMap[m_createInfo.imageType],
            .format = imageFormat,
            .subresourceRange = {.levelCount = m_createInfo.mipLevels},
        };
        m_device->createImageView(createInfo, &m_imageViewMap[imageFormat], this);
    }

    return m_imageViewMap[imageFormat];
}
}  // namespace vkl
