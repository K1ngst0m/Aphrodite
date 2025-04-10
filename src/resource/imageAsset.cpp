#include "imageAsset.h"

#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{
//-----------------------------------------------------------------------------
// ImageAsset Implementation
//-----------------------------------------------------------------------------

ImageAsset::ImageAsset()
    : m_pImageResource(nullptr)
    , m_loadFlags(ImageFeatureBits::eNone)
    , m_containerType(ImageContainerType::eDefault)
    , m_isFromCache(false)
    , m_loadTimestamp(0)
{
}

ImageAsset::~ImageAsset()
{
    // The Resource loader is responsible for freeing the image resource
}

vk::Image* ImageAsset::getImage() const
{
    return m_pImageResource;
}

vk::ImageView* ImageAsset::getView(Format format) const
{
    if (m_pImageResource != nullptr)
    {
        return m_pImageResource->getView(format);
    }
    return nullptr;
}

void ImageAsset::setImageResource(vk::Image* pImage)
{
    m_pImageResource = pImage;
}

void ImageAsset::setLoadInfo(const std::string& sourcePath, const std::string& debugName, const std::string& cacheKey,
                             ImageFeatureFlags flags, ImageContainerType containerType, bool isFromCache)
{
    m_sourcePath    = sourcePath;
    m_debugName     = debugName;
    m_cacheKey      = cacheKey;
    m_loadFlags     = flags;
    m_containerType = containerType;
    m_isFromCache   = isFromCache;
    m_loadTimestamp = std::chrono::steady_clock::now().time_since_epoch().count();
}

std::string ImageAsset::getFormatString() const
{
    if (m_pImageResource == nullptr)
    {
        return "Unknown";
    }

    Format format = m_pImageResource->getFormat();
    switch (format)
    {
    case Format::R8_UNORM:
        return "R8_UNORM";
    case Format::RG8_UNORM:
        return "RG8_UNORM";
    case Format::RGB8_UNORM:
        return "RGB8_UNORM";
    case Format::RGBA8_UNORM:
        return "RGBA8_UNORM";
    case Format::BC1_UNORM:
        return "BC1_UNORM";
    case Format::BC3_UNORM:
        return "BC3_UNORM";
    case Format::BC5_UNORM:
        return "BC5_UNORM";
    case Format::BC7_UNORM:
        return "BC7_UNORM";
    default:
        return "Format_" + std::to_string(static_cast<int>(format));
    }
}

std::string ImageAsset::getTypeString() const
{
    if (m_pImageResource == nullptr)
    {
        return "Unknown";
    }

    if (isCubemap())
    {
        return "Cubemap";
    }

    if (getDepth() > 1)
    {
        return "3D";
    }

    if (getArraySize() > 1)
    {
        return "2D Array";
    }

    return "2D";
}

std::string ImageAsset::getInfoString() const
{
    std::stringstream ss;

    // Basic image properties
    ss << "Image: " << (m_debugName.empty() ? "Unnamed" : m_debugName) << "\n";
    ss << "Dimensions: " << getWidth() << "x" << getHeight();

    if (getDepth() > 1)
    {
        ss << "x" << getDepth();
    }

    if (getArraySize() > 1)
    {
        ss << " (Array: " << getArraySize() << ")";
    }

    ss << "\n";

    // Format and type
    ss << "Format: " << getFormatString() << "\n";
    ss << "Type: " << getTypeString() << "\n";

    // Mipmap info
    ss << "Mipmaps: " << (hasMipmaps() ? std::to_string(getMipLevels()) : "None") << "\n";

    // Source info
    ss << "Source: " << (m_sourcePath.empty() ? "Unknown" : m_sourcePath) << "\n";
    ss << "Cache Key: " << m_cacheKey << "\n";
    ss << "Loaded From Cache: " << (m_isFromCache ? "Yes" : "No") << "\n";

    ss << "Container: ";
    switch (m_containerType)
    {
    case ImageContainerType::ePng:
        ss << "PNG";
        break;
    case ImageContainerType::eJpg:
        ss << "JPEG";
        break;
    case ImageContainerType::eKtx:
        ss << "KTX";
        break;
    case ImageContainerType::eKtx2:
        ss << "KTX2";
        break;
    default:
        ss << "Unknown";
        break;
    }

    // Load flags
    if (m_loadFlags != ImageFeatureBits::eNone)
    {
        ss << "\nFlags: ";
        if (m_loadFlags & ImageFeatureBits::eGenerateMips)
            ss << "GenerateMips ";
        if (m_loadFlags & ImageFeatureBits::eFlipY)
            ss << "FlipY ";
        if (m_loadFlags & ImageFeatureBits::eCubemap)
            ss << "Cubemap ";
        if (m_loadFlags & ImageFeatureBits::eSRGBCorrection)
            ss << "SRGB ";
        if (m_loadFlags & ImageFeatureBits::eCompressKTX2)
            ss << "KTX2 ";
        if (m_loadFlags & ImageFeatureBits::eUseBasisUniversal)
            ss << "Basis ";
    }

    return ss.str();
}

} // namespace aph

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

// Singleton access
ImageCache& ImageCache::get()
{
    static ImageCache instance;
    return instance;
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

} // namespace aph
