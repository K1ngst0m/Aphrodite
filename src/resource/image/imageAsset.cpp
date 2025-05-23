#include "imageAsset.h"
#include "imageCache.h"

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

auto ImageAsset::getWidth() const -> uint32_t
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getWidth() : 0;
}

auto ImageAsset::getHeight() const -> uint32_t
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getHeight() : 0;
}

auto ImageAsset::getDepth() const -> uint32_t
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getDepth() : 1;
}

auto ImageAsset::getMipLevels() const -> uint32_t
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getMipLevels() : 1;
}

auto ImageAsset::getArraySize() const -> uint32_t
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getLayerCount() : 1;
}

auto ImageAsset::getFormat() const -> Format
{
    return (m_pImageResource != nullptr) ? m_pImageResource->getFormat() : Format::Undefined;
}

auto ImageAsset::getSourcePath() const -> const std::string&
{
    return m_sourcePath;
}

auto ImageAsset::getDebugName() const -> const std::string&
{
    return m_debugName;
}

auto ImageAsset::getCacheKey() const -> const std::string&
{
    return m_cacheKey;
}

auto ImageAsset::getLoadFlags() const -> ImageFeatureFlags
{
    return m_loadFlags;
}

auto ImageAsset::getContainerType() const -> ImageContainerType
{
    return m_containerType;
}

auto ImageAsset::isValid() const -> bool
{
    return m_pImageResource != nullptr;
}

auto ImageAsset::isCubemap() const -> bool
{
    return m_loadFlags & ImageFeatureBits::eCubemap;
}

auto ImageAsset::hasMipmaps() const -> bool
{
    return getMipLevels() > 1;
}

auto ImageAsset::isFromCache() const -> bool
{
    return m_isFromCache;
}

auto ImageAsset::getLoadTimestamp() const -> uint64_t
{
    return m_loadTimestamp;
}

auto ImageAsset::getAspectRatio() const -> float
{
    return getHeight() > 0 ? static_cast<float>(getWidth()) / static_cast<float>(getHeight()) : 1.0F;
}

auto ImageAsset::getImage() const -> vk::Image*
{
    return m_pImageResource;
}

auto ImageAsset::getView(Format format) const -> vk::ImageView*
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

auto ImageAsset::getFormatString() const -> std::string
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

auto ImageAsset::getTypeString() const -> std::string
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

auto ImageAsset::getInfoString() const -> std::string
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
