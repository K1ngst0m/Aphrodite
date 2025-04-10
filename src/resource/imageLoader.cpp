#include "imageLoader.h"

#include "common/common.h"
#include "common/profiler.h"
#include "exception/errorMacros.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "resourceLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include KTX libraries
#include <ktx.h>
#include <ktxvulkan.h>

// Define the VkFormat constants we need for KTX texture handling
// These match the values in Vulkan's vkformat_enum.h
#define VK_FORMAT_UNDEFINED 0
#define VK_FORMAT_R8_UNORM 9
#define VK_FORMAT_R8G8_UNORM 16
#define VK_FORMAT_R8G8B8_UNORM 23
#define VK_FORMAT_R8G8B8A8_UNORM 37
#define VK_FORMAT_BC1_RGB_UNORM_BLOCK 131
#define VK_FORMAT_BC3_UNORM_BLOCK 137
#define VK_FORMAT_BC5_UNORM_BLOCK 141
#define VK_FORMAT_BC7_UNORM_BLOCK 145

namespace aph
{
ImageFormat getFormatFromChannels(int channels);
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI);

// Convert a KTX_error_code to our Result error code
Result convertKtxResult(KTX_error_code ktxResult, const std::string& operation = "");

// Map Vulkan format to our internal ImageFormat
ImageFormat getFormatFromVulkan(VkFormat vkFormat);

// Helper to extract texture data from a ktxTexture
Expected<ImageData*> processKtxTexture(ktxTexture* texture, ThreadSafeObjectPool<ImageData>& pool, bool isFlipY);

// Helper to extract texture data from a ktxTexture2
Expected<ImageData*> processKtxTexture2(ktxTexture2* texture, ThreadSafeObjectPool<ImageData>& pool, bool isFlipY);

//-----------------------------------------------------------------------------
// ImageLoader Implementation
//-----------------------------------------------------------------------------

ImageLoader::ImageLoader(ResourceLoader* pResourceLoader)
    : m_pResourceLoader(pResourceLoader)
{
    auto& fs             = APH_DEFAULT_FILESYSTEM;
    auto cachePathResult = fs.resolvePath("texture_cache://");
    if (!cachePathResult.success())
    {
        CM_LOG_ERR("Failed to resolve texture_cache path: %s", cachePathResult.error().toString().data());
        m_cachePath = "texture_cache";
    }
    else
    {
        m_cachePath = cachePathResult.value();
    }

    auto dirResult = fs.createDirectories("texture_cache://");
    if (!dirResult.success())
    {
        CM_LOG_WARN("Failed to create texture cache directory: %s", dirResult.toString().data());
    }

    // Initialize the image cache with our cache path
    ImageCache::get().setCacheDirectory(m_cachePath);

    CM_LOG_INFO("Image cache directory: %s", m_cachePath.c_str());
}

ImageLoader::~ImageLoader()
{
    // Clear the in-memory cache
    ImageCache::get().clear();
}

Expected<ImageAsset*> ImageLoader::loadFromFile(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Get file path with proper protocol
    const auto& pathStr = std::get<std::string>(info.data);
    std::string resolvedPath;

    // Check if the path already has a protocol
    if (pathStr.find(':') == std::string::npos)
    {
        // Prepend the texture protocol
        resolvedPath = "texture:" + pathStr;
    }
    else
    {
        // Already has a protocol
        resolvedPath = pathStr;
    }

    auto& fs = APH_DEFAULT_FILESYSTEM;
    std::string path = fs.resolvePath(resolvedPath).value();

    // Check if file exists
    if (!fs.exist(resolvedPath))
    {
        return Expected<ImageAsset*>{Result::RuntimeError, "File not found: " + path};
    }

    // Detect file type from extension
    ImageContainerType fileType = detectFileType(pathStr);
    Expected<ImageData*> imageDataResult = nullptr;

    // Process the image based on its file type
    if (fileType == ImageContainerType::eKtx2)
    {
        // Process KTX2 source format
        imageDataResult = processKTX2Source(path, info);
    }
    else
    {
        // Process standard image format with possible caching
        imageDataResult = processStandardFormat(resolvedPath, info);
    }

    // Check if loading was successful
    if (!imageDataResult)
    {
        return Expected<ImageAsset*>{Result::RuntimeError, imageDataResult.error().message};
    }

    // Create and return the asset from the image data
    auto result = createImageResources(imageDataResult.value(), info);
    if (!result)
    {
        // Free the image data since we couldn't create the resource
        m_imageDataPool.free(imageDataResult.value());
    }
    return result;
}

void ImageLoader::destroy(ImageAsset* pImageAsset)
{
    if (pImageAsset != nullptr)
    {
        // Destroy the underlying image resource
        vk::Image* pImage = pImageAsset->getImage();
        if (pImage != nullptr)
        {
            m_pResourceLoader->getDevice()->destroy(pImage);
        }

        // Delete the asset
        m_imageAssetPool.free(pImageAsset);
    }
}

//-----------------------------------------------------------------------------
// Loading Pipeline Implementation
//-----------------------------------------------------------------------------

Expected<ImageData*> ImageLoader::loadFromCache(const std::string& cacheKey)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;
    // Get the cache file path using texture_cache protocol
    std::string cachePathStr = "texture_cache://" + cacheKey + ".ktx2";
    std::string cachePath    = fs.resolvePath(cachePathStr).value();

    // Check if the file exists
    if (!fs.exist(cachePathStr))
    {
        return Expected<ImageData*>{Result::RuntimeError, "Cache file does not exist: " + cachePath};
    }

    // Load the KTX2 file
    auto result = loadKTX2(cachePath);
    if (!result)
    {
        return result;
    }

    // Get the loaded image data and update cache info
    ImageData* pImageData = result.value();
    pImageData->isCached  = true;
    pImageData->cacheKey  = cacheKey;
    pImageData->cachePath = cachePath;

    // Add to the memory cache
    ImageCache::get().addImage(cacheKey, pImageData);

    return pImageData;
}

Expected<ImageData*> ImageLoader::loadFromSource(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Check if we're loading raw data instead of a file
    if (std::holds_alternative<ImageRawData>(info.data))
    {
        return loadRawData(info);
    }

    auto& fs = APH_DEFAULT_FILESYSTEM;
    // Get the file path - if it doesn't start with a protocol, prepend the texture protocol
    const auto& pathStr = std::get<std::string>(info.data);
    std::string resolvedPath;

    // Check if the path already has a protocol
    if (pathStr.find(':') == std::string::npos)
    {
        // Prepend the texture protocol
        resolvedPath = "texture:" + pathStr;
    }
    else
    {
        // Already has a protocol
        resolvedPath = pathStr;
    }

    auto path = fs.resolvePath(resolvedPath).value();

    // Check if the file exists
    if (!fs.exist(resolvedPath))
    {
        return Expected<ImageData*>{Result::RuntimeError, "Image file does not exist: " + path};
    }

    // If container type was specified, use it; otherwise determine from extension
    ImageContainerType containerType = info.containerType;
    if (containerType == ImageContainerType::eDefault)
    {
        containerType = getImageContainerType(pathStr);
        if (containerType == ImageContainerType::eDefault)
        {
            std::string extension = pathStr.substr(pathStr.find_last_of('.'));
            return Expected<ImageData*>{Result::RuntimeError, "Unsupported image file format: " + extension};
        }
    }

    // Special case for cubemaps
    if (info.featureFlags & ImageFeatureBits::eCubemap)
    {
        // TODO: Implement cubemap loading
        // This would extract the base path and append _posx, _negx, etc.
        return Expected<ImageData*>{Result::RuntimeError, "Cubemap loading not implemented yet"};
    }

    // Handle different file formats
    switch (containerType)
    {
    case ImageContainerType::eKtx:
        return loadKTX(info);
    case ImageContainerType::eKtx2:
        return loadKTX2(path);
    case ImageContainerType::ePng:
        return loadPNG(info);
    case ImageContainerType::eJpg:
        return loadJPG(info);
    default:
        return Expected<ImageData*>{Result::RuntimeError, "Unsupported image container type"};
    }
}

//-----------------------------------------------------------------------------
// Format-specific loading methods
//-----------------------------------------------------------------------------

Expected<ImageData*> ImageLoader::loadPNG(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;
    // Get file path with proper protocol
    const auto& pathStr = std::get<std::string>(info.data);
    std::string resolvedPath;

    // Check if the path already has a protocol
    if (pathStr.find(':') == std::string::npos)
    {
        // Prepend the texture protocol
        resolvedPath = "texture:" + pathStr;
    }
    else
    {
        // Already has a protocol
        resolvedPath = pathStr;
    }

    std::string path = fs.resolvePath(resolvedPath).value();

    // Allocate a new ImageData
    ImageData* pImageData = m_imageDataPool.allocate();

    // Check if loading succeeded
    if (!pImageData)
    {
        return Expected<ImageData*>{Result::RuntimeError, "Failed to allocate memory for image data"};
    }

    bool isFlipY = (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone;

    // Load the image using stb_image
    stbi_set_flip_vertically_on_load(static_cast<int>(isFlipY));

    int width    = 0;
    int height   = 0;
    int channels = 0;
    uint8_t* img = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (img == nullptr)
    {
        m_imageDataPool.free(pImageData);
        return Expected<ImageData*>{Result::RuntimeError,
                                    "Failed to load PNG image: " + path + " - " + stbi_failure_reason()};
    }

    // Populate image data
    pImageData->width      = width;
    pImageData->height     = height;
    pImageData->format     = getFormatFromChannels(channels);
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Create base mip level
    ImageMipLevel baseMip;
    baseMip.width    = width;
    baseMip.height   = height;
    baseMip.rowPitch = width * (channels == 3 ? 4 : channels); // Ensure RGBA alignment for RGB

    // Convert RGB to RGBA if needed (ensures consistent format handling)
    if (channels == 3)
    {
        baseMip.data.resize(width * height * 4);
        for (int i = 0; i < width * height; ++i)
        {
            baseMip.data[4 * i + 0] = img[3 * i + 0]; // R
            baseMip.data[4 * i + 1] = img[3 * i + 1]; // G
            baseMip.data[4 * i + 2] = img[3 * i + 2]; // B
            baseMip.data[4 * i + 3] = 255; // A (full opacity)
        }
    }
    else
    {
        baseMip.data.resize(width * height * channels);
        memcpy(baseMip.data.data(), img, width * height * channels);
    }

    pImageData->mipLevels.push_back(std::move(baseMip));

    // Free stb_image data
    stbi_image_free(img);

    return pImageData;
}

Expected<ImageData*> ImageLoader::loadJPG(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;
    // Get file path with proper protocol
    const auto& pathStr = std::get<std::string>(info.data);
    std::string resolvedPath;

    // Check if the path already has a protocol
    if (pathStr.find(':') == std::string::npos)
    {
        // Prepend the texture protocol
        resolvedPath = "texture:" + pathStr;
    }
    else
    {
        // Already has a protocol
        resolvedPath = pathStr;
    }

    std::string path = fs.resolvePath(resolvedPath).value();

    // Allocate a new ImageData
    ImageData* pImageData = m_imageDataPool.allocate();

    // Check if allocation succeeded
    if (!pImageData)
    {
        return Expected<ImageData*>{Result::RuntimeError, "Failed to allocate memory for image data"};
    }

    bool isFlipY = (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone;

    // Load the image using stb_image
    stbi_set_flip_vertically_on_load(static_cast<int>(isFlipY));

    int width    = 0;
    int height   = 0;
    int channels = 0;
    uint8_t* img = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (img == nullptr)
    {
        m_imageDataPool.free(pImageData);
        return Expected<ImageData*>{Result::RuntimeError,
                                    "Failed to load JPG image: " + path + " - " + stbi_failure_reason()};
    }

    // Populate image data
    pImageData->width      = width;
    pImageData->height     = height;
    pImageData->format     = getFormatFromChannels(channels);
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Create base mip level
    ImageMipLevel baseMip;
    baseMip.width    = width;
    baseMip.height   = height;
    baseMip.rowPitch = width * (channels == 3 ? 4 : channels); // Ensure RGBA alignment for RGB

    // Convert RGB to RGBA if needed (ensures consistent format handling)
    if (channels == 3)
    {
        baseMip.data.resize(width * height * 4);
        for (int i = 0; i < width * height; ++i)
        {
            baseMip.data[4 * i + 0] = img[3 * i + 0]; // R
            baseMip.data[4 * i + 1] = img[3 * i + 1]; // G
            baseMip.data[4 * i + 2] = img[3 * i + 2]; // B
            baseMip.data[4 * i + 3] = 255; // A (full opacity)
        }
    }
    else
    {
        baseMip.data.resize(width * height * channels);
        memcpy(baseMip.data.data(), img, width * height * channels);
    }

    pImageData->mipLevels.push_back(std::move(baseMip));

    // Free stb_image data
    stbi_image_free(img);

    return pImageData;
}

Expected<ImageData*> ImageLoader::loadKTX(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;
    // Get file path with proper protocol
    const auto& pathStr = std::get<std::string>(info.data);
    std::string resolvedPath;

    // Check if the path already has a protocol
    if (pathStr.find(':') == std::string::npos)
    {
        // Prepend the texture protocol
        resolvedPath = "texture:" + pathStr;
    }
    else
    {
        // Already has a protocol
        resolvedPath = pathStr;
    }

    std::string path = fs.resolvePath(resolvedPath).value();

    // Determine if we should flip the image vertically
    bool isFlipY = (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone;

    // Create KTX texture from file
    ktxTexture* texture = nullptr;
    KTX_error_code result =
        ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (result != KTX_SUCCESS)
    {
        return Expected<ImageData*>{convertKtxResult(result, "Failed to load KTX file: " + path)};
    }

    // Process the KTX texture into our ImageData format
    auto imageResult = processKtxTexture(texture, m_imageDataPool, isFlipY);

    // Clean up KTX texture
    ktxTexture_Destroy(texture);

    return imageResult;
}

Expected<ImageData*> ImageLoader::loadKTX2(const std::string& path)
{
    APH_PROFILER_SCOPE();

    // Determine if we should flip the image vertically
    // We'll check the original path in the info to see if it has the flip flag
    bool isFlipY = false;

    // Create KTX texture from file
    ktxTexture2* texture = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (result != KTX_SUCCESS)
    {
        return Expected<ImageData*>{convertKtxResult(result, "Failed to load KTX2 file: " + path)};
    }

    // Process the KTX2 texture into our ImageData format
    auto imageResult = processKtxTexture2(texture, m_imageDataPool, isFlipY);

    // Clean up KTX2 texture
    ktxTexture_Destroy(ktxTexture(texture));

    return imageResult;
}

Expected<ImageData*> ImageLoader::loadRawData(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Get the raw image data
    const auto& imageInfo = std::get<ImageRawData>(info.data);

    // Allocate new ImageData
    ImageData* pImageData = m_imageDataPool.allocate();

    // Check if allocation succeeded
    if (!pImageData)
    {
        return Expected<ImageData*>{Result::RuntimeError, "Failed to allocate memory for image data"};
    }

    // Populate image data
    pImageData->width      = imageInfo.width;
    pImageData->height     = imageInfo.height;
    pImageData->depth      = 1;
    pImageData->arraySize  = 1;
    pImageData->format     = ImageFormat::eR8G8B8A8Unorm; // Assume RGBA format for raw data
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Create single mip level
    ImageMipLevel mipLevel;
    mipLevel.width    = imageInfo.width;
    mipLevel.height   = imageInfo.height;
    mipLevel.rowPitch = imageInfo.width * 4; // 4 bytes per pixel for RGBA

    mipLevel.data = imageInfo.data;

    pImageData->mipLevels.push_back(std::move(mipLevel));

    return pImageData;
}

Expected<ImageData*> ImageLoader::loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // TODO: Implement cubemap loading
    return {Result::RuntimeError, "Cubemap loading not implemented yet"};
}

// Helper function to determine container type from file extension
ImageContainerType ImageLoader::getImageContainerType(const std::string& path)
{
    APH_PROFILER_SCOPE();

    std::string extension;
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        extension = path.substr(dotPos);
    }

    if (extension == ".ktx2")
    {
        return ImageContainerType::eKtx2;
    }

    if (extension == ".ktx")
    {
        return ImageContainerType::eKtx;
    }

    if (extension == ".png")
    {
        return ImageContainerType::ePng;
    }

    if (extension == ".jpg" || extension == ".jpeg")
    {
        return ImageContainerType::eJpg;
    }

    CM_LOG_ERR("Unsupported image format: %s", path.c_str());
    return ImageContainerType::eDefault;
}

// Helper function to generate a cache key based on image load info
std::string ImageLoader::generateCacheKey(const ImageLoadInfo& info) const
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

std::string ImageLoader::getCacheFilePath(const std::string& cacheKey) const
{
    std::string cachePathStr = "texture_cache://" + cacheKey + ".ktx2";
    return APH_DEFAULT_FILESYSTEM.resolvePath(cachePathStr).value();
}

bool ImageLoader::isCached(const std::string& cacheKey) const
{
    // First check if the cache key exists in the ImageCache singleton
    if (ImageCache::get().existsInFileCache(cacheKey))
    {
        return true;
    }
    
    // Double-check by directly verifying if the cache file exists on disk
    std::string cachePath = getCacheFilePath(cacheKey);
    auto& fs = APH_DEFAULT_FILESYSTEM;
    return fs.exist(cachePath);
}

//-----------------------------------------------------------------------------
// Caching Pipeline Implementation
//-----------------------------------------------------------------------------

Expected<ImageData*> ImageLoader::decodeImage(const ImageLoadInfo& info)
{
    // This is effectively the loadFromSource method, we can just call it
    return loadFromSource(info);
}

Expected<ImageData*> ImageLoader::generateMipmaps(ImageData* pImageData, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Skip if no image data or already has mipmaps
    if (!pImageData || pImageData->mipLevels.size() > 1)
    {
        return pImageData;
    }

    // Get base level dimensions
    uint32_t baseWidth  = pImageData->width;
    uint32_t baseHeight = pImageData->height;

    // Calculate number of mip levels
    uint32_t maxDimension = std::max(baseWidth, baseHeight);
    uint32_t mipLevels    = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;

    // Skip if we only have one level
    if (mipLevels <= 1)
    {
        return pImageData;
    }

    // Check if we have the first mip level
    if (pImageData->mipLevels.empty())
    {
        return Expected<ImageData*>{Result::RuntimeError, "Cannot generate mipmaps: base level missing"};
    }

    // Ensure the base level is RGBA8 format (for simplicity)
    if (pImageData->format != ImageFormat::eR8G8B8A8Unorm)
    {
        return Expected<ImageData*>{Result::RuntimeError, "Mipmap generation only supported for RGBA8 images"};
    }

    // Create additional mip levels
    uint32_t componentCount = 4; // RGBA

    for (uint32_t level = 1; level < mipLevels; level++)
    {
        // Calculate dimensions for this level
        uint32_t mipWidth  = std::max(1u, baseWidth >> level);
        uint32_t mipHeight = std::max(1u, baseHeight >> level);

        // Create a new mip level
        ImageMipLevel mipLevel;
        mipLevel.width    = mipWidth;
        mipLevel.height   = mipHeight;
        mipLevel.rowPitch = mipWidth * componentCount;
        mipLevel.data.resize(mipWidth * mipHeight * componentCount);

        // Get pointers to source and destination data
        const uint8_t* srcData = pImageData->mipLevels[level - 1].data.data();
        uint8_t* dstData       = mipLevel.data.data();

        // Calculate source dimensions and row pitch
        uint32_t srcWidth    = pImageData->mipLevels[level - 1].width;
        uint32_t srcHeight   = pImageData->mipLevels[level - 1].height;
        uint32_t srcRowPitch = pImageData->mipLevels[level - 1].rowPitch;

        // Simple box filter for downsampling
        for (uint32_t y = 0; y < mipHeight; y++)
        {
            for (uint32_t x = 0; x < mipWidth; x++)
            {
                // Source coordinates (in the level above)
                uint32_t srcX = x << 1;
                uint32_t srcY = y << 1;

                // Average the 2x2 block of pixels
                for (uint32_t c = 0; c < componentCount; c++)
                {
                    uint32_t sum   = 0;
                    uint32_t count = 0;

                    // Sample up to 4 pixels in a 2x2 block
                    for (uint32_t dy = 0; dy < 2; dy++)
                    {
                        for (uint32_t dx = 0; dx < 2; dx++)
                        {
                            uint32_t sx = srcX + dx;
                            uint32_t sy = srcY + dy;

                            // Skip if outside source image
                            if (sx >= srcWidth || sy >= srcHeight)
                                continue;

                            // Add pixel value to sum
                            sum += srcData[(sy * srcRowPitch) + (sx * componentCount) + c];
                            count++;
                        }
                    }

                    // Calculate average and store in destination
                    dstData[(y * mipLevel.rowPitch) + (x * componentCount) + c] = static_cast<uint8_t>(sum / count);
                }
            }
        }

        // Add this mip level to the image data
        pImageData->mipLevels.push_back(std::move(mipLevel));
    }

    return pImageData;
}

Expected<bool> ImageLoader::encodeToCacheFile(ImageData* pImageData, const std::string& cachePath)
{
    APH_PROFILER_SCOPE();

    if (!pImageData || pImageData->mipLevels.empty())
    {
        return Expected<bool>{Result::RuntimeError, "Invalid image data for caching"};
    }

    // Check if we need to use Basis Universal compression
    bool useBasisCompression = false;
    bool useUASTC            = false;

    // If this ImageData was loaded as part of an asset, it may have cache information
    if (!pImageData->cacheKey.empty())
    {
        /* TODO: Implement findAssetByCache method in ResourceLoader
        if (auto asset = m_pResourceLoader->findAssetByCache(pImageData->cacheKey))
        {
            ImageFeatureFlags flags = asset->getLoadFlags();
            useBasisCompression = (flags & ImageFeatureBits::eUseBasisUniversal) != ImageFeatureBits::eNone;
            useUASTC = (flags & ImageFeatureBits::eCompressKTX2) != ImageFeatureBits::eNone;
        }
        */
    }

    // Create a new ktxTexture2 for KTX2 file format
    ktxTexture2* texture            = nullptr;
    ktxTextureCreateInfo createInfo = {};

    // Set up creation info
    createInfo.glInternalformat = 0; // Ignored for KTX2
    createInfo.vkFormat         = VK_FORMAT_R8G8B8A8_UNORM; // Default format
    createInfo.baseWidth        = pImageData->width;
    createInfo.baseHeight       = pImageData->height;
    createInfo.baseDepth        = pImageData->depth;
    createInfo.numDimensions    = (pImageData->depth > 1) ? 3 : 2;
    createInfo.numLevels        = static_cast<uint32_t>(pImageData->mipLevels.size());
    createInfo.numLayers        = pImageData->arraySize;
    createInfo.numFaces         = 1;
    createInfo.isArray          = pImageData->arraySize > 1 ? KTX_TRUE : KTX_FALSE;
    createInfo.generateMipmaps  = KTX_FALSE; // We already have mip levels

    // Create the KTX2 texture
    KTX_error_code result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);

    if (result != KTX_SUCCESS)
    {
        return Expected<bool>{convertKtxResult(result, "Failed to create KTX2 texture for encoding")};
    }

    // Add the image data for each mip level
    for (uint32_t level = 0; level < pImageData->mipLevels.size(); ++level)
    {
        const auto& mipLevel = pImageData->mipLevels[level];

        // Set the image data for this level
        result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level,
                                               0, // layer
                                               0, // face
                                               mipLevel.data.data(), mipLevel.data.size());

        if (result != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(texture));
            return Expected<bool>{
                convertKtxResult(result, "Failed to set image data for mip level " + std::to_string(level))};
        }
    }

    // If requested, compress the texture with Basis Universal
    if (useBasisCompression)
    {
        ktxBasisParams params = {};
        params.structSize     = sizeof(params);

        if (useUASTC)
        {
            // Use UASTC format (higher quality, larger size)
            params.uastc        = KTX_TRUE;
            params.qualityLevel = KTX_TF_HIGH_QUALITY;

            CM_LOG_INFO("Compressing texture cache using Basis Universal UASTC format: %s", cachePath.c_str());
        }
        else
        {
            // Use ETC1S format (smaller size, lower quality)
            params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;

            CM_LOG_INFO("Compressing texture cache using Basis Universal ETC1S format: %s", cachePath.c_str());
        }

        // Compress the texture
        result = ktxTexture2_CompressBasisEx(texture, &params);

        if (result != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(texture));
            return Expected<bool>{convertKtxResult(result, "Failed to compress texture with Basis Universal")};
        }
    }
    else
    {
        CM_LOG_INFO("Writing uncompressed KTX2 texture to cache: %s", cachePath.c_str());
    }

    // Write the KTX2 file
    result = ktxTexture_WriteToNamedFile(ktxTexture(texture), cachePath.c_str());

    // Clean up the texture
    ktxTexture_Destroy(ktxTexture(texture));

    if (result != KTX_SUCCESS)
    {
        return Expected<bool>{convertKtxResult(result, "Failed to write KTX2 file: " + cachePath)};
    }

    // Record the time encoded
    pImageData->timeEncoded = std::chrono::steady_clock::now().time_since_epoch().count();

    return Expected<bool>(true);
}

//-----------------------------------------------------------------------------
// GPU Resource Creation
//-----------------------------------------------------------------------------

Expected<ImageAsset*> ImageLoader::createImageResources(ImageData* pImageData, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    if (!pImageData || pImageData->mipLevels.empty())
    {
        return Expected<ImageAsset*>{Result::RuntimeError, "Invalid image data for resource creation"};
    }

    // Create a new image asset
    ImageAsset* pImageAsset = m_imageAssetPool.allocate();
    if (!pImageAsset)
    {
        return Expected<ImageAsset*>{Result::RuntimeError, "Failed to allocate image asset"};
    }

    // Create ImageCreateInfo from the loaded data
    vk::ImageCreateInfo createInfo = info.createInfo;

    // If not already specified, set the create info from image data
    if (createInfo.extent.width == 0 || createInfo.extent.height == 0)
    {
        convertToVulkanFormat(*pImageData, createInfo);
    }

    // Set mip levels from the image data
    createInfo.mipLevels = static_cast<uint32_t>(pImageData->mipLevels.size());

    // Set default usage flags if not already specified
    // ALWAYS ensure TRANSFER_DST flag is set to avoid validation errors
    createInfo.usage |= ImageUsage::TransferDst;

    if (createInfo.usage == ImageUsage::TransferDst)
    {
        // If only TransferDst was set, add Sampled as well since most textures need it
        createInfo.usage |= ImageUsage::Sampled;
    }

    // If we have mipmaps, we need TransferSrc usage as well
    if (pImageData->mipLevels.size() > 1)
    {
        createInfo.usage |= ImageUsage::TransferSrc;
    }

    // Access the device and queues
    vk::Device* pDevice       = m_pResourceLoader->getDevice();
    vk::Queue* pTransferQueue = pDevice->getQueue(QueueType::Transfer);
    vk::Queue* pGraphicsQueue = pDevice->getQueue(QueueType::Graphics);

    if (!pDevice || !pTransferQueue || !pGraphicsQueue)
    {
        m_imageAssetPool.free(pImageAsset);
        return Expected<ImageAsset*>{Result::RuntimeError, "Device or queues not available"};
    }

    // Create staging buffer for the base mip level
    vk::Buffer* stagingBuffer = nullptr;
    {
        // Get size of first mip level
        size_t baseDataSize = pImageData->mipLevels[0].data.size();

        vk::BufferCreateInfo bufferCI{
            .size   = baseDataSize,
            .usage  = BufferUsage::TransferSrc,
            .domain = MemoryDomain::Upload,
        };

        auto stagingResult = pDevice->create(bufferCI, std::string{info.debugName} + std::string{"_staging"});
        if (!stagingResult)
        {
            m_imageAssetPool.free(pImageAsset);
            return Expected<ImageAsset*>{
                Result::RuntimeError,
                "Failed to create staging buffer: " + stagingResult.error().message};
        }

        stagingBuffer = stagingResult.value();

        // Map and copy data to staging buffer
        void* pMapped = pDevice->mapMemory(stagingBuffer);
        if (!pMapped)
        {
            pDevice->destroy(stagingBuffer);
            m_imageAssetPool.free(pImageAsset);
            return Expected<ImageAsset*>{Result::RuntimeError, "Failed to map staging buffer memory"};
        }

        std::memcpy(pMapped, pImageData->mipLevels[0].data.data(), baseDataSize);
        pDevice->unMapMemory(stagingBuffer);
    }

    // Create the image
    vk::Image* image = nullptr;
    {
        auto imageCI   = createInfo;
        imageCI.domain = MemoryDomain::Device;

        auto imageResult = pDevice->create(imageCI, info.debugName);
        if (!imageResult)
        {
            pDevice->destroy(stagingBuffer);
            m_imageAssetPool.free(pImageAsset);
            return Expected<ImageAsset*>{
                Result::RuntimeError,
                "Failed to create image: " + imageResult.error().message};
        }

        image = imageResult.value();

        // For simple copy operations, use the transfer queue
        pDevice->executeCommand(pTransferQueue,
                                [&](auto* cmd)
                                {
                                    // Transition from Undefined to CopyDest
                                    vk::ImageBarrier barrier{.pImage             = image,
                                                             .currentState       = ResourceState::Undefined,
                                                             .newState           = ResourceState::CopyDest,
                                                             .queueType          = pTransferQueue->getType(),
                                                             .subresourceBarrier = 0};
                                    cmd->insertBarrier({barrier});

                                    // Create BufferImageCopy info
                                    BufferImageCopy region{
                                        .bufferOffset      = 0,
                                        .bufferRowLength   = 0, // Tightly packed
                                        .bufferImageHeight = 0, // Tightly packed
                                        .imageSubresource  = {.aspectMask     = 1, // Color aspect
                                                              .mipLevel       = 0, // Base mip level
                                                              .baseArrayLayer = 0,
                                                              .layerCount     = 1},
                                        .imageOffset       = {}, // Zero offset
                                        .imageExtent       = {.width  = pImageData->width,
                                                              .height = pImageData->height,
                                                              .depth  = pImageData->depth}
                                    };

                                    // Copy from staging buffer to image
                                    cmd->copy(stagingBuffer, image, {region});
                                });

        // If we have multiple mip levels
        if (pImageData->mipLevels.size() > 1)
        {
            // For pre-generated mipmaps, upload each level individually
            for (uint32_t i = 1; i < pImageData->mipLevels.size(); i++)
            {
                // Create a staging buffer for this mip level
                size_t mipDataSize = pImageData->mipLevels[i].data.size();

                vk::BufferCreateInfo mipBufferCI{
                    .size   = mipDataSize,
                    .usage  = BufferUsage::TransferSrc,
                    .domain = MemoryDomain::Upload,
                };

                std::string mipStagingName = info.debugName + "_mip" + std::to_string(i) + "_staging";
                auto mipStagingResult      = pDevice->create(mipBufferCI, mipStagingName);
                if (!mipStagingResult)
                {
                    CM_LOG_WARN("Failed to create staging buffer for mip level %u: %s", i,
                                mipStagingResult.error().message.c_str());
                    continue;
                }

                vk::Buffer* mipStagingBuffer = mipStagingResult.value();

                // Map and copy mip level data
                void* pMapped = pDevice->mapMemory(mipStagingBuffer);
                if (!pMapped)
                {
                    pDevice->destroy(mipStagingBuffer);
                    CM_LOG_WARN("Failed to map staging buffer for mip level %u", i);
                    continue;
                }

                std::memcpy(pMapped, pImageData->mipLevels[i].data.data(), mipDataSize);
                pDevice->unMapMemory(mipStagingBuffer);

                // Get dimensions for this mip level
                uint32_t mipWidth  = pImageData->mipLevels[i].width;
                uint32_t mipHeight = pImageData->mipLevels[i].height;

                // Upload this mip level
                pDevice->executeCommand(
                    pTransferQueue,
                    [&](auto* cmd)
                    {
                        // Prepare this mip level for copy
                        vk::ImageBarrier barrier{
                            .pImage       = image,
                            .currentState = ResourceState::Undefined, // First use of each new mip level is Undefined
                            .newState     = ResourceState::CopyDest,
                            .queueType    = pTransferQueue->getType(),
                            .subresourceBarrier = 1, // Target only this specific mip level
                            .mipLevel           = static_cast<uint8_t>(i),
                        };
                        cmd->insertBarrier({barrier});

                        // Create BufferImageCopy for this mip level
                        BufferImageCopy region{
                            .bufferOffset      = 0,
                            .bufferRowLength   = 0, // Tightly packed
                            .bufferImageHeight = 0, // Tightly packed
                            .imageSubresource  = {.aspectMask     = 1, // Color aspect
                                                  .mipLevel       = i,
                                                  .baseArrayLayer = 0,
                                                  .layerCount     = 1},
                            .imageOffset       = {}, // Zero offset
                            .imageExtent       = {.width = mipWidth, .height = mipHeight, .depth = 1}
                        };

                        // Copy mip level data
                        cmd->copy(mipStagingBuffer, image, {region});
                    });

                // Cleanup this mip staging buffer
                pDevice->destroy(mipStagingBuffer);
            }

            // Final transition to ShaderResource for all mip levels
            pDevice->executeCommand(pTransferQueue,
                                    [&](auto* cmd)
                                    {
                                        vk::ImageBarrier finalBarrier{
                                            .pImage             = image,
                                            .currentState       = ResourceState::CopyDest,
                                            .newState           = ResourceState::ShaderResource,
                                            .queueType          = pTransferQueue->getType(),
                                            .subresourceBarrier = 0, // 0 means apply to all mip levels
                                        };
                                        cmd->insertBarrier({finalBarrier});
                                    });
        }
        else
        {
            // Simple final transition for non-mipmapped images
            pDevice->executeCommand(pTransferQueue,
                                    [&](auto* cmd)
                                    {
                                        vk::ImageBarrier finalBarrier{
                                            .pImage             = image,
                                            .currentState       = ResourceState::CopyDest,
                                            .newState           = ResourceState::ShaderResource,
                                            .queueType          = pTransferQueue->getType(),
                                            .subresourceBarrier = 0, // Apply to all mip levels
                                        };
                                        cmd->insertBarrier({finalBarrier});
                                    });
        }
    }

    // Cleanup staging buffer
    pDevice->destroy(stagingBuffer);

    // Set the image in the asset
    pImageAsset->setImageResource(image);

    return pImageAsset;
}

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

ImageFormat getFormatFromChannels(int channels)
{
    switch (channels)
    {
    case 1:
        return ImageFormat::eR8Unorm;
    case 2:
        return ImageFormat::eR8G8Unorm;
    case 3:
        return ImageFormat::eR8G8B8Unorm;
    case 4:
        return ImageFormat::eR8G8B8A8Unorm;
    default:
        return ImageFormat::eUnknown;
    }
}

// Converts our internal format to Vulkan format for API consumption
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI)
{
    APH_PROFILER_SCOPE();

    outCI.extent = {
        .width  = imageData.width,
        .height = imageData.height,
        .depth  = imageData.depth,
    };

    outCI.arraySize = imageData.arraySize;

    // Convert our internal format to Vulkan format
    switch (imageData.format)
    {
    case ImageFormat::eR8Unorm:
        outCI.format = Format::R8_UNORM;
        break;
    case ImageFormat::eR8G8Unorm:
        outCI.format = Format::RG8_UNORM;
        break;
    case ImageFormat::eR8G8B8Unorm:
        // RGB8 not directly supported, we'll use RGBA8
        outCI.format = Format::RGBA8_UNORM;
        break;
    case ImageFormat::eR8G8B8A8Unorm:
        outCI.format = Format::RGBA8_UNORM;
        break;
    case ImageFormat::eBC1RgbUnorm:
        outCI.format = Format::BC1_UNORM;
        break;
    case ImageFormat::eBC3RgbaUnorm:
        outCI.format = Format::BC3_UNORM;
        break;
    case ImageFormat::eBC5RgUnorm:
        outCI.format = Format::BC5_UNORM;
        break;
    case ImageFormat::eBC7RgbaUnorm:
        outCI.format = Format::BC7_UNORM;
        break;
    case ImageFormat::eUASTC4x4:
        // Map UASTC to a supported format
        outCI.format = Format::BC7_UNORM;
        break;
    case ImageFormat::eETC1S:
        // Map ETC1S to a supported format
        outCI.format = Format::BC1_UNORM;
        break;
    default:
        CM_LOG_WARN("Unknown image format, defaulting to RGBA8_UNORM");
        outCI.format = Format::RGBA8_UNORM;
        break;
    }

    // Set mip levels from the image data
    outCI.mipLevels = static_cast<uint32_t>(imageData.mipLevels.size());

    // Set default image type based on dimensions
    if (imageData.depth > 1)
    {
        outCI.imageType = ImageType::e3D;
    }
    else if (imageData.height > 1)
    {
        outCI.imageType = ImageType::e2D;
    }
    else
    {
        outCI.imageType = ImageType::e1D;
    }
}

Result convertKtxResult(KTX_error_code ktxResult, const std::string& operation)
{
    if (ktxResult == KTX_SUCCESS)
        return Result::Success;

    std::string errorMessage = operation.empty() ? "KTX error: " : operation + ": ";

    switch (ktxResult)
    {
    case KTX_FILE_DATA_ERROR:
        errorMessage += "The data in the file is inconsistent with the spec";
        break;
    case KTX_FILE_OPEN_FAILED:
        errorMessage += "The file could not be opened";
        break;
    case KTX_FILE_OVERFLOW:
        errorMessage += "The file size is too large";
        break;
    case KTX_FILE_READ_ERROR:
        errorMessage += "An error occurred while reading the file";
        break;
    case KTX_FILE_SEEK_ERROR:
        errorMessage += "An error occurred while seeking in the file";
        break;
    case KTX_FILE_UNEXPECTED_EOF:
        errorMessage += "Unexpected end of file";
        break;
    case KTX_FILE_WRITE_ERROR:
        errorMessage += "An error occurred while writing to the file";
        break;
    case KTX_GL_ERROR:
        errorMessage += "A GL error occurred";
        break;
    case KTX_INVALID_OPERATION:
        errorMessage += "The operation is not valid for the current state";
        break;
    case KTX_INVALID_VALUE:
        errorMessage += "A parameter was invalid";
        break;
    case KTX_NOT_FOUND:
        errorMessage += "The requested item was not found";
        break;
    case KTX_OUT_OF_MEMORY:
        errorMessage += "Not enough memory to complete the operation";
        break;
    case KTX_TRANSCODE_FAILED:
        errorMessage += "Basis Universal transcoding failed";
        break;
    case KTX_UNKNOWN_FILE_FORMAT:
        errorMessage += "The file not in KTX format";
        break;
    case KTX_UNSUPPORTED_TEXTURE_TYPE:
        errorMessage += "The texture type is not supported by this library";
        break;
    case KTX_UNSUPPORTED_FEATURE:
        errorMessage += "A feature requested is not available in this implementation";
        break;
    default:
        errorMessage += "Unknown error code: " + std::to_string(ktxResult);
        break;
    }

    return {Result::RuntimeError, errorMessage};
}

ImageFormat getFormatFromVulkan(VkFormat vkFormat)
{
    switch (vkFormat)
    {
    case VK_FORMAT_R8_UNORM:
        return ImageFormat::eR8Unorm;
    case VK_FORMAT_R8G8_UNORM:
        return ImageFormat::eR8G8Unorm;
    case VK_FORMAT_R8G8B8_UNORM:
        return ImageFormat::eR8G8B8Unorm;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return ImageFormat::eR8G8B8A8Unorm;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return ImageFormat::eBC1RgbUnorm;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return ImageFormat::eBC3RgbaUnorm;
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return ImageFormat::eBC5RgUnorm;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return ImageFormat::eBC7RgbaUnorm;
    default:
        CM_LOG_WARN("Unsupported VkFormat %d, defaulting to R8G8B8A8_UNORM", static_cast<int>(vkFormat));
        return ImageFormat::eR8G8B8A8Unorm;
    }
}

Expected<ImageData*> processKtxTexture(ktxTexture* texture, ThreadSafeObjectPool<ImageData>& pool, bool isFlipY)
{
    if (texture == nullptr)
    {
        return {Result::RuntimeError, "Invalid KTX texture pointer"};
    }

    // Allocate a new ImageData
    ImageData* pImageData = pool.allocate();
    if (pImageData == nullptr)
    {
        return {Result::RuntimeError, "Failed to allocate memory for image data"};
    }

    // Determine if it's a cubemap
    bool isCubemap = texture->isCubemap;

    // Get basic texture information
    pImageData->width     = texture->baseWidth;
    pImageData->height    = texture->baseHeight;
    pImageData->depth     = (texture->numDimensions == 3) ? texture->baseDepth : 1;
    pImageData->arraySize = isCubemap ? 6 : texture->numLayers;

    // Convert format to our internal format
    VkFormat vkFormat  = ktxTexture_GetVkFormat(texture);
    pImageData->format = getFormatFromVulkan(vkFormat);

    // Set timestamp
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Process all mip levels
    for (uint32_t level = 0; level < texture->numLevels; ++level)
    {
        // Create a new mip level
        ImageMipLevel mipLevel;

        // Calculate dimensions for this mip level
        mipLevel.width  = std::max(1U, texture->baseWidth >> level);
        mipLevel.height = std::max(1U, texture->baseHeight >> level);

        // Get the image size at this level
        ktx_size_t levelSize = ktxTexture_GetImageSize(texture, level);

        // Get image data offset for the first face/layer
        ktx_size_t offset     = 0;
        KTX_error_code result = ktxTexture_GetImageOffset(texture, level, 0, 0, &offset);

        if (result != KTX_SUCCESS)
        {
            pool.free(pImageData);
            return Expected<ImageData*>{
                convertKtxResult(result, "Failed to get image offset for level " + std::to_string(level))};
        }

        // Get pointer to image data
        uint8_t* levelData = ktxTexture_GetData(texture) + offset;

        // Calculate row pitch (assuming tightly packed rows)
        // This might need adjustment based on the format
        mipLevel.rowPitch = mipLevel.width * 4; // Assuming RGBA8 or similar 4-byte format for now

        // Copy the image data
        mipLevel.data.resize(levelSize);
        std::memcpy(mipLevel.data.data(), levelData, levelSize);

        // Handle vertical flipping if needed
        if (isFlipY)
        {
            // Create temporary buffer for flipping
            std::vector<uint8_t> flippedData(levelSize);

            // Flip the image vertically
            for (uint32_t y = 0; y < mipLevel.height; ++y)
            {
                uint32_t srcRow = mipLevel.height - 1 - y;
                std::memcpy(flippedData.data() + y * mipLevel.rowPitch,
                            mipLevel.data.data() + srcRow * mipLevel.rowPitch, mipLevel.rowPitch);
            }

            // Swap with the flipped data
            mipLevel.data.swap(flippedData);
        }

        // Add this mip level to the image data
        pImageData->mipLevels.push_back(std::move(mipLevel));
    }

    return pImageData;
}

Expected<ImageData*> processKtxTexture2(ktxTexture2* texture, ThreadSafeObjectPool<ImageData>& pool, bool isFlipY)
{
    if (!texture)
    {
        return {Result::RuntimeError, "Invalid KTX2 texture pointer"};
    }

    // If texture is Basis compressed, we need to transcode it
    if (ktxTexture2_NeedsTranscoding(texture))
    {
        // Choose optimal target format based on device capabilities
        // For this example, we'll transcode to BC7 for UASTC and BC1/BC3 for ETC1S
        ktx_transcode_fmt_e targetFormat = KTX_TTF_BC7_RGBA;

        if (texture->supercompressionScheme == KTX_SS_BASIS_LZ)
        {
            // For ETC1S, use BC1 for RGB and BC3 for RGBA
            if (ktxTexture2_GetNumComponents(texture) <= 3)
                targetFormat = KTX_TTF_BC1_RGB;
            else
                targetFormat = KTX_TTF_BC3_RGBA;
        }

        // Transcode the texture
        KTX_error_code result = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
        if (result != KTX_SUCCESS)
        {
            return Expected<ImageData*>{convertKtxResult(result, "Failed to transcode KTX2 texture")};
        }
    }

    // Allocate a new ImageData
    ImageData* pImageData = pool.allocate();
    if (!pImageData)
    {
        return Expected<ImageData*>{Result::RuntimeError, "Failed to allocate memory for image data"};
    }

    // Get basic texture information
    pImageData->width     = texture->baseWidth;
    pImageData->height    = texture->baseHeight;
    pImageData->depth     = (texture->numDimensions == 3) ? texture->baseDepth : 1;
    pImageData->arraySize = texture->numLayers;

    // Convert format to our internal format
    VkFormat vkFormat  = ktxTexture2_GetVkFormat(texture);
    pImageData->format = getFormatFromVulkan(vkFormat);

    // Determine image format based on supercompression scheme
    if (texture->supercompressionScheme == KTX_SS_BASIS_LZ)
    {
        pImageData->format = ImageFormat::eETC1S;
    }
    else if (texture->supercompressionScheme == KTX_SS_ZSTD)
    {
        // Format is already set above based on the Vulkan format
    }

    // Set timestamp
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Process all mip levels
    for (uint32_t level = 0; level < texture->numLevels; ++level)
    {
        // Create a new mip level
        ImageMipLevel mipLevel;

        // Calculate dimensions for this mip level
        mipLevel.width  = std::max(1u, texture->baseWidth >> level);
        mipLevel.height = std::max(1u, texture->baseHeight >> level);

        // Get the image size at this level (for the first face/layer)
        ktx_size_t levelSize = ktxTexture_GetImageSize(ktxTexture(texture), level);

        // Get image data offset for the first face/layer
        ktx_size_t offset     = 0;
        KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture(texture), level, 0, 0, &offset);

        if (result != KTX_SUCCESS)
        {
            pool.free(pImageData);
            return Expected<ImageData*>{
                convertKtxResult(result, "Failed to get image offset for level " + std::to_string(level))};
        }

        // Get pointer to image data
        uint8_t* levelData = ktxTexture_GetData(ktxTexture(texture)) + offset;

        // Calculate row pitch (may need adjustment based on the format)
        // For compressed formats, this is the compressed block size
        mipLevel.rowPitch = mipLevel.width * 4; // Assuming RGBA8 for now

        // Copy the image data
        mipLevel.data.resize(levelSize);
        std::memcpy(mipLevel.data.data(), levelData, levelSize);

        // Handle vertical flipping if needed
        if (isFlipY && !ktxTexture2_NeedsTranscoding(texture)) // Only flip if not compressed
        {
            // Create temporary buffer for flipping
            std::vector<uint8_t> flippedData(levelSize);

            // Flip the image vertically
            for (uint32_t y = 0; y < mipLevel.height; ++y)
            {
                uint32_t srcRow = mipLevel.height - 1 - y;
                std::memcpy(flippedData.data() + y * mipLevel.rowPitch,
                            mipLevel.data.data() + srcRow * mipLevel.rowPitch, mipLevel.rowPitch);
            }

            // Swap with the flipped data
            mipLevel.data.swap(flippedData);
        }

        // Add this mip level to the image data
        pImageData->mipLevels.push_back(std::move(mipLevel));
    }

    return pImageData;
}

// Helper to detect file type from extension
ImageContainerType ImageLoader::detectFileType(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.'));
    
    if (extension == ".ktx2" || extension == ".KTX2")
    {
        return ImageContainerType::eKtx2;
    }
    else if (extension == ".ktx" || extension == ".KTX")
    {
        return ImageContainerType::eKtx;
    }
    else if (extension == ".png" || extension == ".PNG")
    {
        return ImageContainerType::ePng;
    }
    else if (extension == ".jpg" || extension == ".jpeg" || extension == ".JPG" || extension == ".JPEG")
    {
        return ImageContainerType::eJpg;
    }
    
    return ImageContainerType::eDefault;
}

// Process KTX2 format with feature analysis
Expected<ImageData*> ImageLoader::processKTX2Source(const std::string& path, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    
    // Check if we should force reload
    bool forceReload = (info.featureFlags & ImageFeatureBits::eForceReload) != ImageFeatureBits::eNone;
    
    if (!forceReload)
    {
        // Try to load from cache first
        auto cacheKey = generateCacheKey(info);
        std::string cachePath = getCacheFilePath(cacheKey);
        
        // Verify that the cache file actually exists
        auto& fs = APH_DEFAULT_FILESYSTEM;
        bool cacheExists = fs.exist(cachePath);
        
        if (cacheExists || isCached(cacheKey))
        {
            CM_LOG_INFO("Loading KTX2 texture from cache: %s", cachePath.c_str());
            return loadFromCache(cacheKey);
        }
        else
        {
            CM_LOG_INFO("Cache miss for KTX2 texture: %s", cachePath.c_str());
        }
    }
    
    // Create KTX texture from file
    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        path.c_str(), 
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, 
        &texture
    );

    if (result != KTX_SUCCESS)
    {
        return Expected<ImageData*>{convertKtxResult(result, "Failed to load KTX2 file: " + path)};
    }

    // Ensure proper cleanup in case of early return
    auto textureGuard = std::unique_ptr<ktxTexture2, std::function<void(ktxTexture2*)>>(
        texture, 
        [](ktxTexture2* p) { if(p) ktxTexture_Destroy(ktxTexture(p)); }
    );
    
    // Analyze KTX2 features
    bool hasMipmaps = texture->numLevels > 1;
    bool needsTranscoding = ktxTexture2_NeedsTranscoding(texture);
    bool isFormatCompatible = true; // Default to true, we'll check device compatibility later if needed
    
    CM_LOG_INFO(
        "KTX2 texture %s: mipmaps=%s, basis=%s", 
        path.c_str(),
        hasMipmaps ? "yes" : "no",
        needsTranscoding ? "yes" : "no"
    );
    
    // Direct load case - already has mipmaps and either doesn't need transcoding or format is compatible
    if (hasMipmaps && (!needsTranscoding || isFormatCompatible))
    {
        CM_LOG_INFO("Using KTX2 texture directly (optimal format): %s", path.c_str());
        return processKtxTexture2(texture, m_imageDataPool, 
                                  (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
    }
    
    // Transcode Basis if needed
    if (needsTranscoding)
    {
        // Choose optimal target format based on device capabilities
        ktx_transcode_fmt_e targetFormat = KTX_TTF_BC7_RGBA;
        
        // For ETC1S, use BC1 for RGB and BC3 for RGBA
        if (texture->supercompressionScheme == KTX_SS_BASIS_LZ)
        {
            if (ktxTexture2_GetNumComponents(texture) <= 3)
                targetFormat = KTX_TTF_BC1_RGB;
            else
                targetFormat = KTX_TTF_BC3_RGBA;
        }
        
        // Transcode to target format
        result = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
        if (result != KTX_SUCCESS)
        {
            return Expected<ImageData*>{convertKtxResult(result, "Failed to transcode KTX2 texture")};
        }
    }
    
    // Generate mipmaps if needed
    if (!hasMipmaps && (info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
    {
        // Process the texture to our format first
        auto imageData = processKtxTexture2(texture, m_imageDataPool, 
                                          (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
        if (!imageData)
        {
            return imageData;
        }
        
        // Generate mipmaps
        auto mipmappedResult = generateMipmaps(imageData.value(), info);
        if (!mipmappedResult)
        {
            m_imageDataPool.free(imageData.value());
            return mipmappedResult;
        }
        
        // Cache the enhanced version
        std::string cacheKey = generateCacheKey(info);
        std::string cachePath = getCacheFilePath(cacheKey);
        
        // Double-check if it's already cached to avoid redundant writes
        auto& fs = APH_DEFAULT_FILESYSTEM;
        if (!fs.exist(cachePath))
        {
            auto cacheResult = encodeToCacheFile(imageData.value(), cachePath);
            if (!cacheResult)
            {
                CM_LOG_WARN("Failed to cache enhanced KTX2 texture: %s", path.c_str());
                // Continue anyway - caching failure is not fatal
            }
            else
            {
                CM_LOG_INFO("Cached enhanced KTX2 texture: %s", cachePath.c_str());
                
                // Update cache info in the image data
                imageData.value()->isCached = true;
                imageData.value()->cacheKey = cacheKey;
                imageData.value()->cachePath = cachePath;
                
                // Add to the memory cache
                ImageCache::get().addImage(cacheKey, imageData.value());
            }
        }
        else
        {
            CM_LOG_INFO("Using existing cached KTX2 texture: %s", cachePath.c_str());
        }
        
        return imageData;
    }
    
    // If we reached here, just return the processed texture
    return processKtxTexture2(texture, m_imageDataPool, 
                            (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
}

// Process standard image formats with caching
Expected<ImageData*> ImageLoader::processStandardFormat(const std::string& resolvedPath, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    
    // Check if we should force reload
    bool forceReload = (info.featureFlags & ImageFeatureBits::eForceReload) != ImageFeatureBits::eNone;
    
    if (!forceReload)
    {
        // Try to load from cache first
        auto cacheKey = generateCacheKey(info);
        std::string cachePath = getCacheFilePath(cacheKey);
        
        // Verify that the cache file actually exists
        auto& fs = APH_DEFAULT_FILESYSTEM;
        bool cacheExists = fs.exist(cachePath);
        
        if (cacheExists || isCached(cacheKey))
        {
            CM_LOG_INFO("Loading from cache: %s", cachePath.c_str());
            
            // Return the cached image data
            return loadFromCache(cacheKey);
        }
        else
        {
            CM_LOG_INFO("Cache miss for: %s", cachePath.c_str());
        }
    }
    
    // Determine file type and load appropriate format
    ImageContainerType containerType = detectFileType(resolvedPath);
    Expected<ImageData*> imageDataResult = nullptr;
    
    switch (containerType)
    {
    case ImageContainerType::ePng:
        imageDataResult = loadPNG(info);
        break;
    case ImageContainerType::eJpg:
        imageDataResult = loadJPG(info);
        break;
    case ImageContainerType::eKtx:
        imageDataResult = loadKTX(info);
        break;
    default:
        return Expected<ImageData*>{Result::RuntimeError, "Unsupported image format"};
    }
    
    // Check if loading was successful
    if (!imageDataResult)
    {
        return imageDataResult;
    }
    
    // Generate mipmaps if requested
    if ((info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
    {
        auto genResult = generateMipmaps(imageDataResult.value(), info);
        if (!genResult)
        {
            m_imageDataPool.free(imageDataResult.value());
            return genResult;
        }
    }
    
    // Cache the result if caching is enabled
    if (!forceReload)
    {
        std::string cacheKey = generateCacheKey(info);
        std::string cachePath = getCacheFilePath(cacheKey);
        
        auto cacheResult = encodeToCacheFile(imageDataResult.value(), cachePath);
        if (!cacheResult)
        {
            CM_LOG_WARN("Failed to write texture to cache: %s", cachePath.c_str());
            // Continue anyway - caching failure is not fatal
        }
        else
        {
            CM_LOG_INFO("Cached texture: %s", cachePath.c_str());
            
            // Update cache info in the image data
            imageDataResult.value()->isCached = true;
            imageDataResult.value()->cacheKey = cacheKey;
            imageDataResult.value()->cachePath = cachePath;
            
            // Add to the memory cache
            ImageCache::get().addImage(cacheKey, imageDataResult.value());
        }
    }
    
    return imageDataResult;
}

} // namespace aph
