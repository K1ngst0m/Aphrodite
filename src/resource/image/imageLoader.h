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
    Expected<ImageAsset*> loadFromFile(const ImageLoadInfo& info);

    // Check if an image exists in cache
    bool isCached(const std::string& cacheKey) const;

    // Get cache file path for an image source
    std::string getCacheFilePath(const std::string& sourceFile) const;

    // Destroy an image asset
    void destroy(ImageAsset* pImageAsset);

private:
    // Caching pipeline steps
    Expected<ImageData*> decodeImage(const ImageLoadInfo& info);
    Expected<ImageData*> generateMipmaps(ImageData* pImageData, const ImageLoadInfo& info);
    Expected<bool> encodeToCacheFile(ImageData* pImageData, const std::string& cachePath);

    // Loading pipeline steps
    Expected<ImageData*> loadFromCache(const std::string& cacheKey);
    Expected<ImageData*> loadFromSource(const ImageLoadInfo& info);

    // Format-specific loading functions
    Expected<ImageData*> loadPNG(const ImageLoadInfo& info);
    Expected<ImageData*> loadJPG(const ImageLoadInfo& info);
    Expected<ImageData*> loadKTX(const ImageLoadInfo& info);
    Expected<ImageData*> loadRawData(const ImageLoadInfo& info);
    Expected<ImageData*> loadKTX2(const std::string& path);

    // Helper for creating cubemap from 6 individual images
    Expected<ImageData*> loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info);

    // Process image data and create GPU resources
    Expected<ImageAsset*> createImageResources(ImageData* pImageData, const ImageLoadInfo& info);

    // Helper function to determine container type from file extension
    ImageContainerType getImageContainerType(const std::string& path);

    // Hash function for cache key generation
    std::string generateCacheKey(const ImageLoadInfo& info) const;

    // Helper function to determine container type from file extension
    ImageContainerType detectFileType(const std::string& path);
    
    // Process KTX2 source with feature analysis
    Expected<ImageData*> processKTX2Source(const std::string& path, const ImageLoadInfo& info);
    
    // Process standard image formats with caching
    Expected<ImageData*> processStandardFormat(const std::string& resolvedPath, const ImageLoadInfo& info);

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<ImageAsset> m_imageAssetPool;
    ThreadSafeObjectPool<ImageData> m_imageDataPool;
    std::string m_cachePath;
};
} // namespace aph
