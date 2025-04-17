#pragma once

#include "api/vulkan/image.h"
#include "common/enum.h"
#include "common/result.h"

namespace aph
{
// Image loading options
enum class ImageFeatureBits : uint8_t
{
    eNone              = 0,
    eGenerateMips      = 1 << 0,
    eFlipY             = 1 << 1,
    eCubemap           = 1 << 2,
    eSRGBCorrection    = 1 << 3,
    eForceReload       = 1 << 4, // Skip cache check
    eCompressKTX2      = 1 << 5, // Use KTX2 compression
    eUseBasisUniversal = 1 << 6, // Use Basis Universal compression
    eForceCPUMipmaps   = 1 << 7, // Force CPU-based mipmap generation
};
using ImageFeatureFlags = Flags<ImageFeatureBits>;

template <>
struct FlagTraits<ImageFeatureBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ImageFeatureFlags allFlags =
        ImageFeatureBits::eGenerateMips | ImageFeatureBits::eFlipY | ImageFeatureBits::eCubemap |
        ImageFeatureBits::eSRGBCorrection | ImageFeatureBits::eForceReload | ImageFeatureBits::eCompressKTX2 |
        ImageFeatureBits::eUseBasisUniversal | ImageFeatureBits::eForceCPUMipmaps;
};

enum class ImageContainerType : uint8_t
{
    eDefault = 0,
    eKtx,
    eKtx2,
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

    // Optional cache control parameters
    std::string cacheKey; // Custom cache key (if empty, one will be generated)
    bool forceUncached = false; // When true, skip cache check
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
    eBC7RgbaUnorm,
    // Add other BASIS/KTX2 compatible formats
    eUASTC4x4,
    eETC1S
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

    // Cache metadata
    bool isCached = false;
    std::string cacheKey;
    std::string cachePath;

    // Timing information
    uint64_t timeLoaded  = 0;
    uint64_t timeEncoded = 0;
};

// Forward declaration for ImageCache (now defined in imageCache.h)
class ImageCache;

class ImageAsset
{
public:
    // Construction/Destruction
    ImageAsset();
    ImageAsset(const ImageAsset&)            = default;
    ImageAsset(ImageAsset&&)                 = delete;
    ImageAsset& operator=(const ImageAsset&) = default;
    ImageAsset& operator=(ImageAsset&&)      = delete;
    ~ImageAsset();

    // Core resource access
    auto getImage() const -> vk::Image*;
    auto getView(Format format = Format::Undefined) const -> vk::ImageView*;
    auto isValid() const -> bool;

    // Image properties
    auto getWidth() const -> uint32_t;
    auto getHeight() const -> uint32_t;
    auto getDepth() const -> uint32_t;
    auto getMipLevels() const -> uint32_t;
    auto getArraySize() const -> uint32_t;
    auto getFormat() const -> Format;
    auto getAspectRatio() const -> float;

    // Image features
    auto isCubemap() const -> bool;
    auto hasMipmaps() const -> bool;
    auto isFromCache() const -> bool;
    auto getLoadFlags() const -> ImageFeatureFlags;
    auto getContainerType() const -> ImageContainerType;

    // Resource metadata
    auto getSourcePath() const -> const std::string&;
    auto getDebugName() const -> const std::string&;
    auto getCacheKey() const -> const std::string&;
    auto getLoadTimestamp() const -> uint64_t;

    // Debug utilities
    auto getFormatString() const -> std::string;
    auto getTypeString() const -> std::string;
    auto getInfoString() const -> std::string;

    // Internal resource management
    void setImageResource(vk::Image* pImage);
    void setLoadInfo(const std::string& sourcePath, const std::string& debugName, const std::string& cacheKey,
                     ImageFeatureFlags flags, ImageContainerType containerType, bool isFromCache);

private:
    vk::Image* m_pImageResource;

    std::string m_sourcePath; // Original source (file path or description)
    std::string m_debugName; // Debug name used for the resource
    std::string m_cacheKey; // Cache lookup key
    ImageFeatureFlags m_loadFlags;
    ImageContainerType m_containerType;
    bool m_isFromCache; // Whether loaded from cache
    uint64_t m_loadTimestamp; // When the image was loaded
};
} // namespace aph
