#include "image.h"
#include "device.h"

#include "api/vulkan/vkUtils.h"

namespace aph::vk
{

Image::Image(Device* pDevice, const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
    , m_pDevice(pDevice)
{
}

Image::~Image()
{
    for (auto& [_, imageView] : m_imageViewFormatMap)
    {
        m_pDevice->destroy(imageView);
    }
}

auto Image::getView(Format imageFormat) -> ImageView*
{
    if (imageFormat == Format::Undefined)
    {
        imageFormat = m_createInfo.format;
    }

    std::lock_guard<std::mutex> holder{m_acquireViewLock};
    if (!m_imageViewFormatMap.contains(imageFormat))
    {
        static const HashMap<ImageType, ImageViewType> imageTypeMap{
            {ImageType::e1D, ImageViewType::e1D},
            {ImageType::e2D, ImageViewType::e2D},
            {ImageType::e3D, ImageViewType::e3D}
        };

        ImageViewCreateInfo createInfo{
            .viewType = imageTypeMap.at(m_createInfo.imageType),
            .format   = imageFormat,
            .pImage   = this,
        };
        createInfo.subresourceRange.setAspectMask(utils::getImageAspect(m_createInfo.format))
            .setLevelCount(m_createInfo.mipLevels)
            .setLayerCount(m_createInfo.arraySize);

        // cubemap
        if (m_createInfo.usage & ImageUsage::CubeCompatible && m_createInfo.arraySize == 6)
        {
            createInfo.viewType = ImageViewType::Cube;
        }
        auto result = m_pDevice->create(createInfo);
        APH_VERIFY_RESULT(result);
        m_imageViewFormatMap[imageFormat] = result.value();
    }

    return m_imageViewFormatMap[imageFormat];
}

ImageView::ImageView(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
    , m_image(createInfo.pImage)
{
}
auto Image::getWidth() const -> uint32_t
{
    return m_createInfo.extent.width;
}
auto Image::getHeight() const -> uint32_t
{
    return m_createInfo.extent.height;
}
auto Image::getDepth() const -> uint32_t
{
    return m_createInfo.extent.depth;
}
auto Image::getMipLevels() const -> uint32_t
{
    return m_createInfo.mipLevels;
}
auto Image::getLayerCount() const -> uint32_t
{
    return m_createInfo.arraySize;
}
auto Image::getFormat() const -> Format
{
    return m_createInfo.format;
}
auto ImageView::getFormat() const -> Format
{
    return m_createInfo.format;
}
auto ImageView::getImageViewType() const -> ImageViewType
{
    return m_createInfo.viewType;
}
auto ImageView::getImage() -> Image*
{
    return m_image;
}
} // namespace aph::vk
