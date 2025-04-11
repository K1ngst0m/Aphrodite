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
    
    // Find image in memory cache
    ImageData* findImage(const std::string& cacheKey);

    // Check if image exists in file cache
    bool existsInFileCache(const std::string& cacheKey) const;

    // Get cache file path
    std::string getCacheFilePath(const std::string& cacheKey) const;

    // Add image to memory cache
    void addImage(const std::string& cacheKey, ImageData* pImageData);

    // Remove image from memory cache
    void removeImage(const std::string& cacheKey);

    // Set cache directory
    void setCacheDirectory(const std::string& path);

    // Get cache directory
    std::string getCacheDirectory() const;

    // Clear memory cache (doesn't affect file cache)
    void clear();

    // Generate a cache key based on image load info
    std::string generateCacheKey(const ImageLoadInfo& info) const;

private:
    std::string m_cacheDirectory;
    HashMap<std::string, ImageData*> m_memoryCache;
    mutable std::mutex m_cacheMutex;
};
} // namespace aph