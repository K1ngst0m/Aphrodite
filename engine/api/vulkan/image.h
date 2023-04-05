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
    Extent3D extent;
    uint32_t flags = 0;
    ImageType imageType = IMAGE_TYPE_2D;
    uint32_t alignment = 0;
    uint32_t mipLevels = 1;
    uint32_t layerCount = 1;
    uint32_t arrayLayers = 1;
    ImageUsageFlags usage;
    MemoryPropertyFlags property;
    Format format;
    SampleCountFlags samples = SAMPLE_COUNT_1_BIT;
    ImageTiling tiling = IMAGE_TILING_OPTIMAL;
};

class VulkanImage : public ResourceHandle<VkImage, ImageCreateInfo>
{
public:
    VulkanImage(VulkanDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, VkDeviceMemory memory = VK_NULL_HANDLE);
    ~VulkanImage();

    VkDeviceMemory getMemory() { return m_memory; }
    VulkanDevice *getDevice() { return m_device; }

    VkResult bind(VkDeviceSize offset = 0) const;

    VulkanImageView *getImageView(Format imageFormat = FORMAT_UNDEFINED);

    Extent3D getExtent() const { return m_createInfo.extent; }
    uint32_t getWidth() const { return m_createInfo.extent.width; }
    uint32_t getHeight() const { return m_createInfo.extent.height; }
    uint32_t getMipLevels() const { return m_createInfo.mipLevels; }
    uint32_t getLayerCount() const { return m_createInfo.layerCount; }
    uint32_t getOffset() const { return m_createInfo.alignment; }

private:
    VulkanDevice *m_device = nullptr;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    std::unordered_map<uint32_t, VulkanImageView*> m_imageViewMap;

    void *m_mapped = nullptr;
};

struct ImageViewCreateInfo
{
    ImageViewType viewType;
    ImageViewDimension dimension;
    Format format;
    ComponentMapping components;
    ImageSubresourceRange subresourceRange;
};

class VulkanImageView : public ResourceHandle<VkImageView, ImageViewCreateInfo>
{
public:
    VulkanImageView(const ImageViewCreateInfo &createInfo, VulkanImage *pImage, VkImageView handle)
        : m_device(pImage->getDevice()), m_image(pImage)
    {
        getHandle() = handle;
        getCreateInfo() = createInfo;
    }

    ImageViewType getImageViewType() const { return m_createInfo.viewType; }
    Format getFormat() const { return m_createInfo.format; }
    ComponentMapping getComponentMapping() const { return m_createInfo.components; }

    const ImageSubresourceRange &GetSubresourceRange() const { return m_createInfo.subresourceRange; }
    VulkanImage *getImage() { return m_image; }
    VulkanDevice *getDevice() { return m_device; }

    VkDescriptorImageInfo & getDescInfoMap(VkImageLayout layout){
        if(!m_descInfoMap.count(layout))
        {
            m_descInfoMap[layout] = {
                .sampler = VK_NULL_HANDLE,
                .imageView = getHandle(),
                .imageLayout = layout,
            };
        }
        return m_descInfoMap[layout];
    }

private:
    VulkanDevice *m_device {};
    VulkanImage *m_image {};
    std::unordered_map<VkImageLayout, VkDescriptorImageInfo> m_descInfoMap;
};

}  // namespace aph

#endif  // IMAGE_H_
