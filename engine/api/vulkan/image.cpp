#include "image.h"
#include "api/vulkan/vkUtils.h"
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
        static const std::unordered_map<VkImageType, VkImageViewType> imageTypeMap{
            {VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D},
        };
        ImageViewCreateInfo createInfo{
            .viewType         = imageTypeMap.at(m_createInfo.imageType),
            .format           = imageFormat,
            .subresourceRange = {
                .aspectMask = utils::getImageAspect(m_createInfo.format),
                .baseMipLevel = 0,
                .levelCount = m_createInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = m_createInfo.layerCount},
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
