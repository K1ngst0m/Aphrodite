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
    Extent3D extent      = {};
    uint32_t alignment   = { 0 };
    uint32_t mipLevels   = { 1 };
    uint32_t arraySize   = { 1 };
    uint32_t sampleCount = { 1 };

    ImageUsageFlags usage = {};

    MemoryDomain domain = { MemoryDomain::Auto };
    ImageType imageType = { ImageType::e2D };
    Format format       = { Format::Undefined };
};

class Image : public ResourceHandle<::vk::Image, ImageCreateInfo>
{
    friend class CommandBuffer;
    friend class ThreadSafeObjectPool<Image>;

public:
    auto getView(Format imageFormat = Format::Undefined) -> ImageView*;

    auto getWidth() const -> uint32_t;
    auto getHeight() const -> uint32_t;
    auto getDepth() const -> uint32_t;
    auto getMipLevels() const -> uint32_t;
    auto getLayerCount() const -> uint32_t;
    auto getFormat() const -> Format;

private:
    Image(Device* pDevice, const CreateInfoType& createInfo, HandleType handle);
    ~Image();

    Device* m_pDevice                                = {};
    HashMap<Format, ImageView*> m_imageViewFormatMap = {};
    std::mutex m_acquireViewLock;
};

struct ImageViewCreateInfo
{
    ImageViewType viewType                       = { ImageViewType::e2D };
    Format format                                = { Format::Undefined };
    ::vk::ComponentMapping components            = {};
    ::vk::ImageSubresourceRange subresourceRange = { ::vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    Image* pImage                                = {};
};

class ImageView : public ResourceHandle<::vk::ImageView, ImageViewCreateInfo>
{
    friend class ThreadSafeObjectPool<ImageView>;

public:
    auto getFormat() const -> Format;
    auto getImageViewType() const -> ImageViewType;
    auto getImage() -> Image*;

private:
    ImageView(const CreateInfoType& createInfo, HandleType handle);
    Image* m_image = {};
};

} // namespace aph::vk
