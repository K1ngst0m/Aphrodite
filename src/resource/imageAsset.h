#pragma once

#include "api/vulkan/image.h"
#include "common/enum.h"

namespace aph
{
// Image loading options
enum class ImageFeatureBits : uint8_t
{
    eNone           = 0,
    eGenerateMips   = 1 << 0,
    eFlipY          = 1 << 1,
    eCubemap        = 1 << 2,
    eSRGBCorrection = 1 << 3,
};
using ImageFeatureFlags = Flags<ImageFeatureBits>;

template <>
struct FlagTraits<ImageFeatureBits>
{
    static constexpr bool isBitmask             = true;
    static constexpr ImageFeatureFlags allFlags = ImageFeatureBits::eGenerateMips | ImageFeatureBits::eFlipY |
                                                  ImageFeatureBits::eCubemap | ImageFeatureBits::eSRGBCorrection;
};

enum class ImageContainerType : uint8_t
{
    eDefault = 0,
    eKtx,
    ePng,
    eJpg,
};

struct ImageRawData
{
    uint32_t width  = {};
    uint32_t height = {};
    std::vector<uint8_t> data;
};

// Load info structure for images
struct ImageLoadInfo
{
    std::string debugName;
    std::variant<std::string, ImageRawData> data;
    ImageContainerType containerType = {ImageContainerType::eDefault};
    vk::ImageCreateInfo createInfo   = {};
    ImageFeatureFlags featureFlags   = ImageFeatureBits::eNone;
};

enum class ImageFormat : uint8_t
{
    eUnknown,
    eR8Unorm,
    eR8G8Unorm,
    eR8G8B8Unorm,
    eR8G8B8A8Unorm,
    eBC1RgbUnorm,
    eBC3RgbaUnorm,
    eBC5RgUnorm,
    eBC7RgbaUnorm
};

struct ImageMipLevel
{
    uint32_t width;
    uint32_t height;
    uint32_t rowPitch;
    std::vector<uint8_t> data;
};

struct ImageData
{
    uint32_t width     = 0;
    uint32_t height    = 0;
    uint32_t depth     = 1;
    uint32_t arraySize = 1;
    ImageFormat format = ImageFormat::eUnknown;
    SmallVector<ImageMipLevel> mipLevels;
};

// Singleton image cache manager
class ImageCache
{
public:
    static ImageCache& get();

    std::shared_ptr<ImageData> findImage(const std::string& path);
    void addImage(const std::string& path, std::shared_ptr<ImageData> imageData);
    void clear();

private:
    ImageCache() = default;
    HashMap<std::string, std::shared_ptr<ImageData>> m_cache;
};

class ImageAsset
{
public:
    ImageAsset();
    ImageAsset(const ImageAsset&)            = default;
    ImageAsset(ImageAsset&&)                 = delete;
    ImageAsset& operator=(const ImageAsset&) = default;
    ImageAsset& operator=(ImageAsset&&)      = delete;
    ~ImageAsset();

    // Accessors
    uint32_t getWidth() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getWidth() : 0;
    }
    uint32_t getHeight() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getHeight() : 0;
    }
    uint32_t getDepth() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getDepth() : 1;
    }
    uint32_t getMipLevels() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getMipLevels() : 1;
    }
    uint32_t getArraySize() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getLayerCount() : 1;
    }
    Format getFormat() const
    {
        return (m_pImageResource != nullptr) ? m_pImageResource->getFormat() : Format::Undefined;
    }

    // Mid-level loading info accessors
    const std::string& getSourcePath() const
    {
        return m_sourcePath;
    }
    const std::string& getDebugName() const
    {
        return m_debugName;
    }
    ImageFeatureFlags getLoadFlags() const
    {
        return m_loadFlags;
    }
    ImageContainerType getContainerType() const
    {
        return m_containerType;
    }
    bool isValid() const
    {
        return m_pImageResource != nullptr;
    }
    bool isCubemap() const
    {
        return m_loadFlags & ImageFeatureBits::eCubemap;
    }
    bool hasMipmaps() const
    {
        return getMipLevels() > 1;
    }
    uint64_t getLoadTimestamp() const
    {
        return m_loadTimestamp;
    }

    // Utility methods
    float getAspectRatio() const
    {
        return getHeight() > 0 ? static_cast<float>(getWidth()) / static_cast<float>(getHeight()) : 1.0F;
    }
    std::string getFormatString() const;
    std::string getTypeString() const;

    // Get metadata as a formatted string (useful for debug overlays)
    std::string getInfoString() const;

    // Resource access
    vk::Image* getImage() const;
    vk::ImageView* getView(Format format = Format::Undefined) const;

    // Internal use by the image loader
    void setImageResource(vk::Image* pImage);
    void setLoadInfo(const std::string& sourcePath, const std::string& debugName, ImageFeatureFlags flags,
                     ImageContainerType containerType);

private:
    vk::Image* m_pImageResource;

    std::string m_sourcePath; // Original source (file path or description)
    std::string m_debugName; // Debug name used for the resource
    ImageFeatureFlags m_loadFlags;
    ImageContainerType m_containerType;
    uint64_t m_loadTimestamp; // When the image was loaded
};
} // namespace aph
