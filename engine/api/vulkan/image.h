#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_

#include "vkUtils.h"

namespace aph::vk
{
class Device;
class ImageView;

struct ImageCreateInfo
{
    Extent3D extent      = {};
    uint32_t flags       = {0};
    uint32_t alignment   = {0};
    uint32_t mipLevels   = {1};
    uint32_t arraySize   = {1};
    uint32_t sampleCount = {1};

    VkImageUsageFlags  usage   = {};
    ImageDomain        domain  = {ImageDomain::Device};
    VkSampleCountFlags samples = {VK_SAMPLE_COUNT_1_BIT};

    VkImageType imageType = {VK_IMAGE_TYPE_2D};
    Format      format    = {Format::Undefined};
};

class Image : public ResourceHandle<VkImage, ImageCreateInfo>
{
    friend class CommandBuffer;
    friend class ObjectPool<Image>;

public:
    ImageView* getView(Format imageFormat = Format::Undefined);

    uint32_t      getWidth() const { return m_createInfo.extent.width; }
    uint32_t      getHeight() const { return m_createInfo.extent.height; }
    uint32_t      getDepth() const { return m_createInfo.extent.depth; }
    uint32_t      getMipLevels() const { return m_createInfo.mipLevels; }
    uint32_t      getLayerCount() const { return m_createInfo.arraySize; }
    Format        getFormat() const { return m_createInfo.format; }
    ResourceState getResourceState() const { return m_resourceState; }

private:
    Image(Device* pDevice, const CreateInfoType& createInfo, HandleType handle);
    ~Image();

    Device*                                m_pDevice            = {};
    std::unordered_map<Format, ImageView*> m_imageViewFormatMap = {};
    VkImageLayout                          m_layout             = {VK_IMAGE_LAYOUT_UNDEFINED};
    ResourceState                          m_resourceState      = {};
};

struct ImageViewCreateInfo
{
    VkImageViewType         viewType         = {VK_IMAGE_VIEW_TYPE_2D};
    Format                  format           = {Format::Undefined};
    VkComponentMapping      components       = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    Image*                  pImage           = {};
};

class ImageView : public ResourceHandle<VkImageView, ImageViewCreateInfo>
{
    friend class ObjectPool<ImageView>;
public:
    Format                  getFormat() const { return m_createInfo.format; }
    VkImageViewType         getImageViewType() const { return m_createInfo.viewType; }
    VkImageSubresourceRange GetSubresourceRange() const { return m_createInfo.subresourceRange; }
    VkComponentMapping      getComponentMapping() const { return m_createInfo.components; }

    Image* getImage() { return m_image; }

private:
    ImageView(const CreateInfoType& createInfo, HandleType handle);

    Image*                                                   m_image       = {};
    std::unordered_map<VkImageLayout, VkDescriptorImageInfo> m_descInfoMap = {};
};

}  // namespace aph::vk

#endif  // IMAGE_H_
