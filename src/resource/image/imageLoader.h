#pragma once

#include "imageAsset.h"
#include "imageCache.h"

class ktxTexture;
class ktxTexture2;
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

    Expected<ImageAsset*> load(const ImageLoadInfo& info);
    void unload(ImageAsset* pImageAsset);

private:
    Expected<ImageData*> loadFromCache(const std::string& cacheKey);
    Expected<ImageData*> loadFromSource(const ImageLoadInfo& info);

    Expected<ImageData*> loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info);
    Expected<ImageData*> processKTX2Source(const std::string& path, const ImageLoadInfo& info);
    Expected<ImageData*> processStandardFormat(const std::string& resolvedPath, const ImageLoadInfo& info);

    Expected<ImageData*> loadPNG(const ImageLoadInfo& info);
    Expected<ImageData*> loadJPG(const ImageLoadInfo& info);
    Expected<ImageData*> loadKTX(const ImageLoadInfo& info);
    Expected<ImageData*> loadRawData(const ImageLoadInfo& info);
    Expected<ImageData*> loadKTX2(const std::string& path);

    // Helper for processing KTX textures
    Expected<ImageData*> processKtxTexture(ktxTexture* texture, bool isFlipY);
    Expected<ImageData*> processKtxTexture2(ktxTexture2* texture, bool isFlipY);

    Expected<ImageAsset*> createImageResources(ImageData* pImageData, const ImageLoadInfo& info);

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<ImageAsset> m_imageAssetPool;
    ThreadSafeObjectPool<ImageData> m_imageDataPool;
    std::string m_cachePath;
    ImageCache m_imageCache;
};
} // namespace aph
