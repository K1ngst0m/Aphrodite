#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct ImageCreateInfo
{
    Extent3D extent = {};
    uint32_t alignment = { 0 };
    uint32_t mipLevels = { 1 };
    uint32_t arraySize = { 1 };
    uint32_t sampleCount = { 1 };

    ImageUsageFlags usage = {};

    MemoryDomain domain = { MemoryDomain::Auto };
    ImageType imageType = { ImageType::e2D };
    Format format = { Format::Undefined };
};

class Image : public ResourceHandle<::vk::Image, ImageCreateInfo>
{
    friend class CommandBuffer;
    friend class ObjectPool<Image>;

public:
    ImageView* getView(Format imageFormat = Format::Undefined);

    uint32_t getWidth() const
    {
        return m_createInfo.extent.width;
    }
    uint32_t getHeight() const
    {
        return m_createInfo.extent.height;
    }
    uint32_t getDepth() const
    {
        return m_createInfo.extent.depth;
    }
    uint32_t getMipLevels() const
    {
        return m_createInfo.mipLevels;
    }
    uint32_t getLayerCount() const
    {
        return m_createInfo.arraySize;
    }
    Format getFormat() const
    {
        return m_createInfo.format;
    }
    ResourceState getResourceState() const
    {
        return m_resourceState;
    }

private:
    Image(Device* pDevice, const CreateInfoType& createInfo, HandleType handle);
    ~Image();

    Device* m_pDevice = {};
    HashMap<Format, ImageView*> m_imageViewFormatMap = {};
    ::vk::ImageLayout m_layout = {};
    ResourceState m_resourceState = {};
    std::mutex m_acquireViewLock;
};

struct ImageViewCreateInfo
{
    ImageViewType viewType = { ImageViewType::e2D };
    Format format = { Format::Undefined };
    ::vk::ComponentMapping components = {};
    ::vk::ImageSubresourceRange subresourceRange = { ::vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    Image* pImage = {};
};

class ImageView : public ResourceHandle<::vk::ImageView, ImageViewCreateInfo>
{
    friend class ObjectPool<ImageView>;

public:
    Format getFormat() const
    {
        return m_createInfo.format;
    }
    ImageViewType getImageViewType() const
    {
        return m_createInfo.viewType;
    }

    Image* getImage()
    {
        return m_image;
    }

private:
    ImageView(const CreateInfoType& createInfo, HandleType handle);

    Image* m_image = {};
    HashMap<::vk::ImageLayout, ::vk::DescriptorImageInfo> m_descInfoMap = {};
};

} // namespace aph::vk
