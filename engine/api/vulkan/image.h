#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;
class VulkanImageView;

struct ImageCreateInfo
{
    Extent3D extent      = {};
    uint32_t flags       = {0};
    uint32_t alignment   = {0};
    uint32_t mipLevels   = {1};
    uint32_t layerCount  = {1};
    uint32_t arrayLayers = {1};

    VkImageUsageFlags     usage    = {0};
    VkMemoryPropertyFlags property = {0};
    VkSampleCountFlags    samples  = {VK_SAMPLE_COUNT_1_BIT};

    ImageType   imageType     = {ImageType::_2D};
    Format      format        = {Format::UNDEFINED};
    ImageTiling tiling        = {ImageTiling::OPTIMAL};
    ImageLayout initialLayout = {ImageLayout::UNDEFINED};
};

class VulkanImage : public ResourceHandle<VkImage, ImageCreateInfo>
{
public:
    VulkanImage(VulkanDevice* pDevice, const ImageCreateInfo& createInfo, VkImage image,
                VkDeviceMemory memory = VK_NULL_HANDLE);
    ~VulkanImage();

    VkDeviceMemory getMemory() { return m_memory; }

    VulkanImageView* getImageView(Format imageFormat = Format::UNDEFINED);

    Extent3D getExtent() const { return m_createInfo.extent; }
    uint32_t getWidth() const { return m_createInfo.extent.width; }
    uint32_t getHeight() const { return m_createInfo.extent.height; }
    uint32_t getMipLevels() const { return m_createInfo.mipLevels; }
    uint32_t getLayerCount() const { return m_createInfo.layerCount; }
    uint32_t getOffset() const { return m_createInfo.alignment; }

private:
    VulkanDevice*                                m_pDevice            = {};
    std::unordered_map<Format, VulkanImageView*> m_imageViewFormatMap = {};
    VkDeviceMemory                               m_memory             = {};
};

struct ImageViewCreateInfo
{
    ImageViewType         viewType         = {ImageViewType::_2D};
    ImageViewDimension    dimension        = {ImageViewDimension::_2D};
    Format                format           = {Format::UNDEFINED};
    ComponentMapping      components       = {};
    ImageSubresourceRange subresourceRange = {};
};

class VulkanImageView : public ResourceHandle<VkImageView, ImageViewCreateInfo>
{
public:
    VulkanImageView(const ImageViewCreateInfo& createInfo, VulkanImage* pImage, VkImageView handle) : m_image(pImage)
    {
        getHandle()     = handle;
        getCreateInfo() = createInfo;
    }

    Format                getFormat() const { return m_createInfo.format; }
    ComponentMapping      getComponentMapping() const { return m_createInfo.components; }
    ImageViewType         getImageViewType() const { return m_createInfo.viewType; }
    ImageSubresourceRange GetSubresourceRange() const { return m_createInfo.subresourceRange; }

    VulkanImage* getImage() { return m_image; }

private:
    VulkanImage*                                             m_image       = {};
    std::unordered_map<VkImageLayout, VkDescriptorImageInfo> m_descInfoMap = {};
};

}  // namespace aph

#endif  // IMAGE_H_
