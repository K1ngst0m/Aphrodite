#pragma once

#include "api/vulkan/device.h"

namespace aph
{
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

struct ImageLoadInfo
{
    std::string debugName = {};
    std::variant<std::string, ImageInfo> data;
    ImageContainerType containerType = { ImageContainerType::Default };
    vk::ImageCreateInfo createInfo = {};
    bool generateMips = false;
    bool isFlipY = false;
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

// Image loading utility functions
std::shared_ptr<ImageData> loadImageFromFile(std::string_view path, bool isFlipY = false);
std::array<std::shared_ptr<ImageData>, 6> loadCubemapFromFiles(const std::array<std::string_view, 6>& paths);
bool loadKTX(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data);
bool loadPNGJPG(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data,
                bool isFlipY = false);

} // namespace aph
