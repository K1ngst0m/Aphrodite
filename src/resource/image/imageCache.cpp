#include "imageCache.h"

#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{
//-----------------------------------------------------------------------------
// ImageCache Implementation
//-----------------------------------------------------------------------------

ImageCache::ImageCache()
    : m_cacheDirectory(APH_DEFAULT_FILESYSTEM.resolvePath("texture_cache://"))
{
    // Create the cache directory if it doesn't exist
    auto dirResult = APH_DEFAULT_FILESYSTEM.createDirectories("texture_cache://");
    if (!dirResult.success())
    {
        CM_LOG_WARN("Failed to create texture cache directory: %s", dirResult.toString().data());
    }
}

ImageData* ImageCache::findImage(const std::string& cacheKey)
{
    APH_PROFILER_SCOPE();

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (auto it = m_memoryCache.find(cacheKey); it != m_memoryCache.end())
    {
        return it->second;
    }
    return nullptr;
}

bool ImageCache::existsInFileCache(const std::string& cacheKey) const
{
    auto cachePath = getCacheFilePath(cacheKey);
    return APH_DEFAULT_FILESYSTEM.exist(cachePath);
}

std::string ImageCache::getCacheFilePath(const std::string& cacheKey) const
{
    return m_cacheDirectory + "/" + cacheKey + ".ktx2";
}

void ImageCache::addImage(const std::string& cacheKey, ImageData* pImageData)
{
    APH_PROFILER_SCOPE();

    if (!pImageData)
        return;

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // If entry already exists, remove the old one first
    auto it = m_memoryCache.find(cacheKey);
    if (it != m_memoryCache.end())
    {
        // Don't delete the old one, just update the mapping
        // The old one might still be in use somewhere
    }

    m_memoryCache[cacheKey] = pImageData;
}

void ImageCache::removeImage(const std::string& cacheKey)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    auto it = m_memoryCache.find(cacheKey);
    if (it != m_memoryCache.end())
    {
        // Just remove from cache, don't delete the object
        // It might still be in use somewhere
        m_memoryCache.erase(it);
    }
}

void ImageCache::setCacheDirectory(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cacheDirectory = path;

    // Create the directory if it doesn't exist
    auto dirResult = APH_DEFAULT_FILESYSTEM.createDirectories(path);
    if (!dirResult.success())
    {
        CM_LOG_WARN("Failed to create cache directory %s: %s", path.c_str(), dirResult.toString().data());
    }
}

std::string ImageCache::getCacheDirectory() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cacheDirectory;
}

void ImageCache::clear()
{
    APH_PROFILER_SCOPE();

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // Only clear the cache mapping, don't delete the objects
    // They might still be in use somewhere
    m_memoryCache.clear();
}

// Helper function to generate a cache key based on image load info
std::string ImageCache::generateCacheKey(const ImageLoadInfo& info) const
{
    // Create a hash combining:
    // - Source path or dimensions for raw data
    // - Feature flags
    // - Format
    std::stringstream ss;

    auto& fs = APH_DEFAULT_FILESYSTEM;

    // Add source info
    if (std::holds_alternative<std::string>(info.data))
    {
        const auto& path = std::get<std::string>(info.data);
        ss << fs.resolvePath(path).value();
    }
    else
    {
        const auto& rawData = std::get<ImageRawData>(info.data);
        ss << "raw_" << rawData.width << "x" << rawData.height << "_" << rawData.data.size();
    }

    // Add feature flags
    ss << "_" << static_cast<uint32_t>(info.featureFlags);

    // Add format if specified
    if (info.createInfo.format != Format::Undefined)
    {
        ss << "_fmt" << static_cast<uint32_t>(info.createInfo.format);
    }

    // Hash the result
    std::string fullString = ss.str();
    size_t hash            = std::hash<std::string>{}(fullString);

    // Return a hex string of the hash
    std::stringstream hashStream;
    hashStream << std::hex << hash;
    return hashStream.str();
}

} // namespace aph