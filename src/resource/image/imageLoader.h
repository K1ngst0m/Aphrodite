#pragma once

#include "imageAsset.h"
#include "imageCache.h"
#include "resource/forward.h"

class ktxTexture;
class ktxTexture2;

namespace aph
{
class ResourceLoader;

class ImageLoader
{
public:
    ImageLoader(const ImageLoader&)                    = delete;
    ImageLoader(ImageLoader&&)                         = delete;
    auto operator=(const ImageLoader&) -> ImageLoader& = delete;
    auto operator=(ImageLoader&&) -> ImageLoader&      = delete;
    explicit ImageLoader(ResourceLoader* pResourceLoader);
    ~ImageLoader();

    auto load(const ImageLoadInfo& info) -> Expected<ImageAsset*>;
    void unload(ImageAsset* pImageAsset);

private:
    auto loadFromCache(const std::string& cacheKey) -> Expected<ImageData*>;
    auto loadFromSource(const ImageLoadInfo& info) -> Expected<ImageData*>;

    auto loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto processKTX2Source(const std::string& path, const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto processStandardFormat(const std::string& resolvedPath, const ImageLoadInfo& info) -> Expected<ImageData*>;

    auto loadPNG(const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto loadJPG(const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto loadKTX(const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto loadRawData(const ImageLoadInfo& info) -> Expected<ImageData*>;
    auto loadKTX2(const std::string& path) -> Expected<ImageData*>;

    auto processKtxTexture(ktxTexture* texture, bool isFlipY) -> Expected<ImageData*>;
    auto processKtxTexture2(ktxTexture2* texture, bool isFlipY) -> Expected<ImageData*>;
    auto createImageResources(ImageData* pImageData, const ImageLoadInfo& info) -> Expected<ImageAsset*>;

private:
    ResourceLoader* m_pResourceLoader = {};
    ThreadSafeObjectPool<ImageAsset> m_imageAssetPool;
    ThreadSafeObjectPool<ImageData> m_imageDataPool;
    std::string m_cachePath;
    ImageCache m_imageCache;
};
} // namespace aph
