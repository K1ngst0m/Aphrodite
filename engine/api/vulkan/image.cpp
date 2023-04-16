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

VulkanImageView* VulkanImage::getImageView(Format imageFormat)
{
    if(imageFormat == Format::UNDEFINED)
    {
        imageFormat = m_createInfo.format;
    }

    if(!m_imageViewFormatMap.count(imageFormat))
    {
        std::unordered_map<ImageType, ImageViewType> imageTypeMap{
            { ImageType::_1D, ImageViewType::_1D },
            { ImageType::_2D, ImageViewType::_2D },
            { ImageType::_2D, ImageViewType::_3D },
        };
        ImageViewCreateInfo createInfo{
            .viewType         = imageTypeMap[m_createInfo.imageType],
            .format           = imageFormat,
            .subresourceRange = { .levelCount = m_createInfo.mipLevels },
        };
        m_pDevice->createImageView(createInfo, &m_imageViewFormatMap[imageFormat], this);
    }

    return m_imageViewFormatMap[imageFormat];
}
}  // namespace aph
