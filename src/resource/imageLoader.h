#pragma once

#include "imageAsset.h"

namespace aph
{
// Forward declarations
class ResourceLoader;

// Image loader class (internal to the resource system)
class ImageLoader
{
public:
    ImageLoader(const ImageLoader&)            = delete;
    ImageLoader(ImageLoader&&)                 = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;
    ImageLoader& operator=(ImageLoader&&)      = delete;
    explicit ImageLoader(ResourceLoader* pResourceLoader);
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
