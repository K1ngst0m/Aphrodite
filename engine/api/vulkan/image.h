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

    VkImageUsageFlags  usage   = {};
    ImageDomain        domain  = {ImageDomain::Device};
    VkSampleCountFlags samples = {VK_SAMPLE_COUNT_1_BIT};

    VkImageType   imageType     = {VK_IMAGE_TYPE_2D};
    VkFormat      format        = {VK_FORMAT_UNDEFINED};
    VkImageTiling tiling        = {VK_IMAGE_TILING_OPTIMAL};
    VkImageLayout initialLayout = {VK_IMAGE_LAYOUT_UNDEFINED};
};

class VulkanImage : public ResourceHandle<VkImage, ImageCreateInfo>
{
public:
    VulkanImage(VulkanDevice* pDevice, const ImageCreateInfo& createInfo, VkImage image,
                VkDeviceMemory memory = VK_NULL_HANDLE);
    ~VulkanImage();

    VkDeviceMemory getMemory() { return m_memory; }

    VulkanImageView* getView(VkFormat imageFormat = VK_FORMAT_UNDEFINED);

    Extent3D getExtent() const { return m_createInfo.extent; }
    uint32_t getWidth() const { return m_createInfo.extent.width; }
    uint32_t getHeight() const { return m_createInfo.extent.height; }
    uint32_t getMipLevels() const { return m_createInfo.mipLevels; }
    uint32_t getLayerCount() const { return m_createInfo.layerCount; }
    uint32_t getOffset() const { return m_createInfo.alignment; }

private:
    VulkanDevice*                                  m_pDevice            = {};
    std::unordered_map<VkFormat, VulkanImageView*> m_imageViewFormatMap = {};
    VkDeviceMemory                                 m_memory             = {};
};

struct ImageViewCreateInfo
{
    VkImageViewType         viewType         = {VK_IMAGE_VIEW_TYPE_2D};
    VkFormat                format           = {VK_FORMAT_UNDEFINED};
    VkComponentMapping      components       = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_NONE, 0, 1, 0, 1};
};

class VulkanImageView : public ResourceHandle<VkImageView, ImageViewCreateInfo>
{
public:
    VulkanImageView(const ImageViewCreateInfo& createInfo, VulkanImage* pImage, VkImageView handle);

    VkFormat                getFormat() const { return m_createInfo.format; }
    VkImageViewType         getImageViewType() const { return m_createInfo.viewType; }
    VkImageSubresourceRange GetSubresourceRange() const { return m_createInfo.subresourceRange; }
    VkComponentMapping      getComponentMapping() const { return m_createInfo.components; }

    VulkanImage* getImage() { return m_image; }

private:
    VulkanImage*                                             m_image       = {};
    std::unordered_map<VkImageLayout, VkDescriptorImageInfo> m_descInfoMap = {};
};

}  // namespace aph

#endif  // IMAGE_H_
