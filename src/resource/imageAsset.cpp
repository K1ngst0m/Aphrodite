#include "imageAsset.h"

namespace aph
{
//-----------------------------------------------------------------------------
// ImageAsset Implementation
//-----------------------------------------------------------------------------

ImageAsset::ImageAsset()
    : m_pImageResource(nullptr)
    , m_loadFlags(ImageFeatureBits::None)
    , m_containerType(ImageContainerType::Default)
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
    if (m_pImageResource)
    {
        return m_pImageResource->getView(format);
    }
    return nullptr;
}

void ImageAsset::setImageResource(vk::Image* pImage)
{
    m_pImageResource = pImage;
}

void ImageAsset::setLoadInfo(const std::string& sourcePath, const std::string& debugName, ImageFeatureFlags flags,
                             ImageContainerType containerType)
{
    m_sourcePath    = sourcePath;
    m_debugName     = debugName;
    m_loadFlags     = flags;
    m_containerType = containerType;
    m_loadTimestamp = std::chrono::steady_clock::now().time_since_epoch().count();
}

std::string ImageAsset::getFormatString() const
{
    if (!m_pImageResource)
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
    if (!m_pImageResource)
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
    ss << "Container: ";
    switch (m_containerType)
    {
    case ImageContainerType::Png:
        ss << "PNG";
        break;
    case ImageContainerType::Jpg:
        ss << "JPEG";
        break;
    case ImageContainerType::Ktx:
        ss << "KTX";
        break;
    default:
        ss << "Unknown";
        break;
    }

    // Load flags
    if (m_loadFlags != ImageFeatureBits::None)
    {
        ss << "\nFlags: ";
        if (m_loadFlags & ImageFeatureBits::GenerateMips)
            ss << "GenerateMips ";
        if (m_loadFlags & ImageFeatureBits::FlipY)
            ss << "FlipY ";
        if (m_loadFlags & ImageFeatureBits::Cubemap)
            ss << "Cubemap ";
        if (m_loadFlags & ImageFeatureBits::SRGBCorrection)
            ss << "SRGB ";
    }

    return ss.str();
}

} // namespace aph

namespace aph
{
//-----------------------------------------------------------------------------
// ImageCache Implementation
//-----------------------------------------------------------------------------

// Singleton access
ImageCache& ImageCache::get()
{
    static ImageCache instance;
    return instance;
}

std::shared_ptr<ImageData> ImageCache::findImage(const std::string& path)
{
    APH_PROFILER_SCOPE();
    auto it = m_cache.find(path);
    if (it != m_cache.end())
    {
        return it->second;
    }
    return nullptr;
}

void ImageCache::addImage(const std::string& path, std::shared_ptr<ImageData> imageData)
{
    APH_PROFILER_SCOPE();
    m_cache[path] = imageData;
}

void ImageCache::clear()
{
    APH_PROFILER_SCOPE();
    m_cache.clear();
}

} // namespace aph
