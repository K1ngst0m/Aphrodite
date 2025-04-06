#pragma once

#include "api/vulkan/device.h"

namespace aph
{
// Forward declarations
class ResourceLoader;

enum class ImageFormat
{
    Unknown,
    R8_UNORM,
    R8G8_UNORM,
    R8G8B8_UNORM,
    R8G8B8A8_UNORM,
    BC1_RGB_UNORM,
    BC3_RGBA_UNORM,
    BC5_RG_UNORM,
    BC7_RGBA_UNORM
};

enum class ImageContainerType
{
    Default = 0,
    Ktx,
    Png,
    Jpg,
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
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t arraySize = 1;
    ImageFormat format = ImageFormat::Unknown;
    SmallVector<ImageMipLevel> mipLevels;
};

struct ImageInfo
{
    uint32_t width = {};
    uint32_t height = {};
    std::vector<uint8_t> data = {};
};

// Image loading options
enum class ImageFeatureBits : uint32_t
{
    None = 0,
    GenerateMips = 1 << 0,
    FlipY = 1 << 1,
    Cubemap = 1 << 2,
    SRGBCorrection = 1 << 3,
};
using ImageFeatureFlags = Flags<ImageFeatureBits>;

template <>
struct FlagTraits<ImageFeatureBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ImageFeatureFlags allFlags = ImageFeatureBits::GenerateMips | ImageFeatureBits::FlipY |
                                                  ImageFeatureBits::Cubemap | ImageFeatureBits::SRGBCorrection;
};

// Load info structure for images
struct ImageLoadInfo
{
    std::string debugName = {};
    std::variant<std::string, ImageInfo> data;
    ImageContainerType containerType = {ImageContainerType::Default};
    vk::ImageCreateInfo createInfo = {};
    ImageFeatureFlags featureFlags = ImageFeatureBits::None;
};

// Mid-level image asset class that manages GPU image resources
class ImageAsset
{
public:
    ImageAsset();
    ~ImageAsset();

    // Accessors

    // Resource access
    vk::Image* getImage() const;

    // Internal use by the image loader
    void setImageResource(vk::Image* pImage);

private:
    vk::Image* m_pImageResource;
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

// Image loader class (internal to the resource system)
class ImageLoader
{
public:
    ImageLoader(ResourceLoader* pResourceLoader);
    ~ImageLoader();

    // Load an image asset from a file or raw data
    Result loadFromFile(const ImageLoadInfo& info, ImageAsset** ppImageAsset);

    // Destroy an image asset
    void destroy(ImageAsset* pImageAsset);

private:
    // Helper loading functions for different formats
    Result loadPNG(const ImageLoadInfo& info, ImageAsset** ppImageAsset);
    Result loadJPG(const ImageLoadInfo& info, ImageAsset** ppImageAsset);
    Result loadKTX(const ImageLoadInfo& info, ImageAsset** ppImageAsset);
    Result loadRawData(const ImageLoadInfo& info, ImageAsset** ppImageAsset);

    // Helper for creating cubemap from 6 individual images
    Result loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info, ImageAsset** ppImageAsset);

    // Process image data and create GPU resources
    Result createImageResources(std::shared_ptr<ImageData> imageData, const ImageLoadInfo& info,
                                ImageAsset** ppImageAsset);

    // Helper function to determine container type from file extension
    ImageContainerType GetImageContainerType(const std::filesystem::path& path);

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<ImageAsset> m_imageAssetPools;
};
} // namespace aph
