#include "image.h"
#include "device.h"

namespace aph
{

VulkanImage::VulkanImage(VulkanDevice* pDevice, const ImageCreateInfo& createInfo, VkImage image,
                         VkDeviceMemory memory) :
    m_pDevice(pDevice),
    m_memory(memory)
{
    getHandle()     = image;
    getCreateInfo() = createInfo;
}

VulkanImage::~VulkanImage()
{
    for(auto& [_, imageView] : m_imageViewFormatMap)
    {
        m_pDevice->destroyImageView(imageView);
    }
}

VulkanImageView* VulkanImage::getView(VkFormat imageFormat)
{
    if(imageFormat == VK_FORMAT_UNDEFINED) { imageFormat = m_createInfo.format; }

    if(!m_imageViewFormatMap.count(imageFormat))
    {
        std::unordered_map<VkImageType, VkImageViewType> imageTypeMap{
            {VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D},
        };
        ImageViewCreateInfo createInfo{
            .viewType         = imageTypeMap[m_createInfo.imageType],
            .format           = imageFormat,
            .subresourceRange = {.levelCount = m_createInfo.mipLevels},
        };
        m_pDevice->createImageView(createInfo, &m_imageViewFormatMap[imageFormat], this);
    }

    return m_imageViewFormatMap[imageFormat];
}

VulkanImageView::VulkanImageView(const ImageViewCreateInfo& createInfo, VulkanImage* pImage, VkImageView handle) :
    m_image(pImage)
{
    getHandle()     = handle;
    getCreateInfo() = createInfo;
}
}  // namespace aph
