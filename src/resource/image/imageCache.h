#pragma once

#include "common/common.h"
#include "imageAsset.h"

namespace aph
{
// Forward declarations
struct ImageData;

// Image cache manager
class ImageCache
{
public:
    // Constructor is now public
    ImageCache();

    // Cache directory management
    void setCacheDirectory(const std::string& path);
    auto getCacheDirectory() const -> std::string;
    auto getCacheFilePath(const std::string& cacheKey) const -> std::string;

    // Memory cache operations
    void addImage(const std::string& cacheKey, ImageData* pImageData);
    void removeImage(const std::string& cacheKey);
    void clear();
    auto findImage(const std::string& cacheKey) -> ImageData*;

    // Cache key and existence checks
    auto existsInFileCache(const std::string& cacheKey) const -> bool;
    auto generateCacheKey(const ImageLoadInfo& info) const -> std::string;

private:
    std::string m_cacheDirectory;
    HashMap<std::string, ImageData*> m_memoryCache;
    mutable std::mutex m_cacheMutex;
};
} // namespace aph