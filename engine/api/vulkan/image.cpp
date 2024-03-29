#include "image.h"
#include "api/vulkan/vkUtils.h"
#include "device.h"

namespace aph::vk
{

Image::Image(Device* pDevice, const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_pDevice(pDevice)
{
}

Image::~Image()
{
    for(auto& [_, imageView] : m_imageViewFormatMap)
    {
        m_pDevice->destroy(imageView);
    }
}

ImageView* Image::getView(Format imageFormat)
{
    if(imageFormat == Format::Undefined)
    {
        imageFormat = m_createInfo.format;
    }

    std::lock_guard<std::mutex> holder{m_acquireViewLock};
    if(!m_imageViewFormatMap.contains(imageFormat))
    {
        static const HashMap<VkImageType, VkImageViewType> imageTypeMap{
            {VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D},
            {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D},
        };
        ImageViewCreateInfo createInfo{
            .viewType         = imageTypeMap.at(m_createInfo.imageType),
            .format           = imageFormat,
            .subresourceRange = {.aspectMask     = utils::getImageAspect(utils::VkCast(m_createInfo.format)),
                                 .baseMipLevel   = 0,
                                 .levelCount     = m_createInfo.mipLevels,
                                 .baseArrayLayer = 0,
                                 .layerCount     = m_createInfo.arraySize},
            .pImage           = this,
        };
        // cubemap
        if(m_createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT && m_createInfo.arraySize == 6)
        {
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }
        APH_VR(m_pDevice->create(createInfo, &m_imageViewFormatMap[imageFormat]));
    }

    return m_imageViewFormatMap[imageFormat];
}

ImageView::ImageView(const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_image(createInfo.pImage)
{
}
}  // namespace aph::vk
