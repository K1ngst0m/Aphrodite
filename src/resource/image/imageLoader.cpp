#include "imageLoader.h"

#include "common/common.h"
#include "common/profiler.h"
#include "exception/errorMacros.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "imageUtil.h"
#include "resource/resourceLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include KTX libraries
#include <ktx.h>
#include <ktxvulkan.h>

namespace aph
{
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
        LOADER_LOG_ERR("Failed to resolve texture_cache path: %s", cachePathResult.error().toString().data());
        m_cachePath = "texture_cache";
    }
    else
    {
        m_cachePath = cachePathResult.value();
    }

    auto dirResult = fs.createDirectories("texture_cache://");
    if (!dirResult.success())
    {
        LOADER_LOG_WARN("Failed to create texture cache directory: %s", dirResult.toString().data());
    }

    // Initialize the image cache with our cache path
    m_imageCache.setCacheDirectory(m_cachePath);

    LOADER_LOG_INFO("Image cache directory: %s", m_cachePath.c_str());
}

ImageLoader::~ImageLoader()
{
    // Clear the in-memory cache
    m_imageCache.clear();
}

Expected<ImageData*> ImageLoader::processKtxTexture(ktxTexture* texture, bool isFlipY)
{
    APH_PROFILER_SCOPE();

    if (texture == nullptr)
    {
        return {Result::RuntimeError, "Invalid KTX texture pointer"};
    }

    // Allocate a new ImageData
    ImageData* pImageData = m_imageDataPool.allocate();
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
        // Fill the mip level data using our utility function
        auto mipLevelResult = fillMipLevel(texture, level, isFlipY, pImageData->width, pImageData->height);

        // Check if mip level data was filled successfully
        if (!mipLevelResult)
        {
            m_imageDataPool.free(pImageData);
            return {Result::RuntimeError, "Failed to fill mip level data"};
        }

        // Add this mip level to the image data
        pImageData->mipLevels.push_back(std::move(mipLevelResult.value()));
    }

    return pImageData;
}

Expected<ImageData*> ImageLoader::processKtxTexture2(ktxTexture2* texture, bool isFlipY)
{
    APH_PROFILER_SCOPE();

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
            return {convertKtxResult(result, "Failed to transcode KTX2 texture")};
        }
    }

    // Allocate a new ImageData
    ImageData* pImageData = m_imageDataPool.allocate();
    if (!pImageData)
    {
        return {Result::RuntimeError, "Failed to allocate memory for image data"};
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
        // Fill the mip level data using our utility function
        auto mipLevelResult = fillMipLevel(texture, level, isFlipY, pImageData->width, pImageData->height);

        // Check if mip level data was filled successfully
        if (!mipLevelResult)
        {
            m_imageDataPool.free(pImageData);
            return {Result::RuntimeError, "Failed to fill mip level data"};
        }

        // Add this mip level to the image data
        pImageData->mipLevels.push_back(std::move(mipLevelResult.value()));
    }

    return pImageData;
}

Expected<ImageAsset*> ImageLoader::load(const ImageLoadInfo& info)
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

    auto& fs         = APH_DEFAULT_FILESYSTEM;
    std::string path = fs.resolvePath(resolvedPath).value();

    // Check if file exists
    if (!fs.exist(resolvedPath))
    {
        return {Result::RuntimeError, "File not found: " + path};
    }

    // Detect file type from extension
    ImageContainerType fileType          = aph::detectFileType(pathStr);
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
        return {Result::RuntimeError, imageDataResult.error().message};
    }

    // Generate mipmaps if requested
    if ((info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
    {
        // For standard format textures, we'll prefer GPU mipmap generation during final resource creation
        // Only generate CPU mipmaps if explicitly requested or for caching purposes
        bool needsCpuMipmaps = false;

        // Check for explicit CPU mipmap flags
        if (info.featureFlags & ImageFeatureBits::eForceCPUMipmaps)
            needsCpuMipmaps = true;

        // Generate CPU mipmaps for caching purposes
        if (needsCpuMipmaps)
        {
            auto genResult = generateMipmaps(imageDataResult.value());
            if (!genResult)
            {
                m_imageDataPool.free(imageDataResult.value());
                return genResult.transform([](bool) { return nullptr; });
            }
        }
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

void ImageLoader::unload(ImageAsset* pImageAsset)
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

    // First check if the image is in memory cache
    if (ImageData* pCachedImage = m_imageCache.findImage(cacheKey))
    {
        return pCachedImage;
    }

    // Get the cache file path
    std::string cachePath = m_imageCache.getCacheFilePath(cacheKey);

    // Check if the file exists
    if (!m_imageCache.existsInFileCache(cacheKey))
    {
        return {Result::RuntimeError, "Cache file does not exist: " + cachePath};
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
    m_imageCache.addImage(cacheKey, pImageData);

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
        return {Result::RuntimeError, "Image file does not exist: " + path};
    }

    // If container type was specified, use it; otherwise determine from extension
    ImageContainerType containerType = info.containerType;
    if (containerType == ImageContainerType::eDefault)
    {
        containerType = aph::detectFileType(pathStr);
        if (containerType == ImageContainerType::eDefault)
        {
            std::string extension = pathStr.substr(pathStr.find_last_of('.'));
            return {Result::RuntimeError, "Unsupported image file format: " + extension};
        }
    }

    // Special case for cubemaps
    if (info.featureFlags & ImageFeatureBits::eCubemap)
    {
        // Extract base path for cubemap faces
        std::string basePath = pathStr;
        size_t dotPos        = basePath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            basePath = basePath.substr(0, dotPos);
        }

        // Construct paths for all six faces with standard suffixes
        std::array<std::string, 6> cubeFaces = {
            basePath + "_posx" + basePath.substr(dotPos), // Positive X
            basePath + "_negx" + basePath.substr(dotPos), // Negative X
            basePath + "_posy" + basePath.substr(dotPos), // Positive Y
            basePath + "_negy" + basePath.substr(dotPos), // Negative Y
            basePath + "_posz" + basePath.substr(dotPos), // Positive Z
            basePath + "_negz" + basePath.substr(dotPos) // Negative Z
        };

        return loadCubemap(cubeFaces, info);
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
        return {Result::RuntimeError, "Unsupported image container type"};
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
        return {Result::RuntimeError, "Failed to allocate memory for image data"};
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
        return {Result::RuntimeError, "Failed to load PNG image: " + path + " - " + stbi_failure_reason()};
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
        return {Result::RuntimeError, "Failed to allocate memory for image data"};
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
        return {Result::RuntimeError, "Failed to load JPG image: " + path + " - " + stbi_failure_reason()};
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
        return {convertKtxResult(result, "Failed to load KTX file: " + path)};
    }

    // Process the KTX texture into our ImageData format
    auto imageResult = processKtxTexture(texture, isFlipY);

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
        return {convertKtxResult(result, "Failed to load KTX2 file: " + path)};
    }

    // Process the KTX2 texture into our ImageData format
    auto imageResult = processKtxTexture2(texture, isFlipY);

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
        return {Result::RuntimeError, "Failed to allocate memory for image data"};
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

    auto& fs = APH_DEFAULT_FILESYSTEM;

    // Check if all face files exist
    for (const auto& path : paths)
    {
        std::string resolvedPath;

        // Check if the path already has a protocol
        if (path.find(':') == std::string::npos)
        {
            // Prepend the texture protocol
            resolvedPath = "texture:" + path;
        }
        else
        {
            // Already has a protocol
            resolvedPath = path;
        }

        if (!fs.exist(resolvedPath))
        {
            return {Result::RuntimeError, "Cubemap face not found: " + path};
        }
    }

    // Allocate new ImageData for the cubemap
    ImageData* pImageData = m_imageDataPool.allocate();
    if (!pImageData)
    {
        return {Result::RuntimeError, "Failed to allocate memory for cubemap data"};
    }

    // Load the first face to determine format and dimensions
    // We'll use this as reference for the other faces
    ImageLoadInfo faceInfo = info;
    faceInfo.featureFlags &= ~ImageFeatureBits::eCubemap; // Remove cubemap flag for individual faces

    // Temporarily store the original data
    auto originalData = faceInfo.data;
    faceInfo.data     = paths[0]; // Set to first face

    auto firstFaceResult = loadFromSource(faceInfo);
    if (!firstFaceResult)
    {
        m_imageDataPool.free(pImageData);
        return firstFaceResult;
    }

    ImageData* pFirstFace = firstFaceResult.value();

    // Initialize cubemap data based on first face
    pImageData->width      = pFirstFace->width;
    pImageData->height     = pFirstFace->height;
    pImageData->depth      = 1;
    pImageData->arraySize  = 6; // 6 faces for cubemap
    pImageData->format     = pFirstFace->format;
    pImageData->timeLoaded = std::chrono::steady_clock::now().time_since_epoch().count();

    // Add the first face data to our cubemap
    pImageData->mipLevels = std::move(pFirstFace->mipLevels);

    // Cleanup first face as we've moved its data
    m_imageDataPool.free(pFirstFace);

    // Load remaining faces and validate they match the first face
    for (size_t i = 1; i < paths.size(); ++i)
    {
        faceInfo.data   = paths[i];
        auto faceResult = loadFromSource(faceInfo);

        if (!faceResult)
        {
            m_imageDataPool.free(pImageData);
            return faceResult;
        }

        ImageData* pFace = faceResult.value();

        // Validate dimensions match
        if (pFace->width != pImageData->width || pFace->height != pImageData->height)
        {
            m_imageDataPool.free(pFace);
            m_imageDataPool.free(pImageData);
            return {Result::RuntimeError, "Cubemap face dimensions don't match: " + paths[i]};
        }

        // Validate format matches
        if (pFace->format != pImageData->format)
        {
            m_imageDataPool.free(pFace);
            m_imageDataPool.free(pImageData);
            return {Result::RuntimeError, "Cubemap face format doesn't match: " + paths[i]};
        }

        // Add face data to cubemap
        // In a real implementation, we'd handle each face properly here
        // For now, we're just adding more mip levels (which is incorrect, but placeholder)
        // The correct implementation would store all faces for each mip level
        if (!pFace->mipLevels.empty())
        {
            pImageData->mipLevels.push_back(std::move(pFace->mipLevels[0]));
        }

        // Cleanup this face
        m_imageDataPool.free(pFace);
    }

    // Restore the original data
    faceInfo.data = originalData;

    // Mark this as a cubemap
    // This would require additional fields in ImageData or some other way to track faces

    return pImageData;
}

//-----------------------------------------------------------------------------
// GPU Resource Creation
//-----------------------------------------------------------------------------

Expected<ImageAsset*> ImageLoader::createImageResources(ImageData* pImageData, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    if (!pImageData || pImageData->mipLevels.empty())
    {
        return {Result::RuntimeError, "Invalid image data for resource creation"};
    }

    // Create a new image asset
    ImageAsset* pImageAsset = m_imageAssetPool.allocate();
    if (!pImageAsset)
    {
        return {Result::RuntimeError, "Failed to allocate image asset"};
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
    if ((createInfo.usage & ImageUsage::TransferDst) == ImageUsage::None)
    {
        createInfo.usage |= ImageUsage::TransferDst;
    }

    // If no usage flags were specified other than TransferDst, add Sampled as well
    if ((createInfo.usage & ~ImageUsage::TransferDst) == ImageUsage::None)
    {
        createInfo.usage |= ImageUsage::Sampled;
    }

    // If we have mipmaps, we need TransferSrc usage as well for potential transitions
    if (pImageData->mipLevels.size() > 1 && (createInfo.usage & ImageUsage::TransferSrc) == ImageUsage::None)
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
        return {Result::RuntimeError, "Device or queues not available"};
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
            return {Result::RuntimeError, "Failed to create staging buffer: " + stagingResult.error().message};
        }

        stagingBuffer = stagingResult.value();

        // Map and copy data to staging buffer
        void* pMapped = pDevice->mapMemory(stagingBuffer);
        if (!pMapped)
        {
            pDevice->destroy(stagingBuffer);
            m_imageAssetPool.free(pImageAsset);
            return {Result::RuntimeError, "Failed to map staging buffer memory"};
        }

        std::memcpy(pMapped, pImageData->mipLevels[0].data.data(), baseDataSize);
        pDevice->unMapMemory(stagingBuffer);
    }

    // Create the image
    vk::Image* image = nullptr;
    {
        auto imageCI   = createInfo;
        imageCI.domain = MemoryDomain::Device;

        // If mipmap generation is needed, ensure proper usage flags
        if (pImageData->mipLevels.size() == 1 &&
            (info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
        {
            // Ensure both TransferSrc and TransferDst flags are set for GPU mipmap generation
            if ((imageCI.usage & ImageUsage::TransferSrc) == ImageUsage::None)
                imageCI.usage |= ImageUsage::TransferSrc;

            if ((imageCI.usage & ImageUsage::TransferDst) == ImageUsage::None)
                imageCI.usage |= ImageUsage::TransferDst;

            // Calculate mipmap levels if not already set
            uint32_t maxDimension = std::max(imageCI.extent.width, imageCI.extent.height);
            imageCI.mipLevels     = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;

            // Update the createInfo for later use
            createInfo.mipLevels = imageCI.mipLevels;
            createInfo.usage     = imageCI.usage;

            LOADER_LOG_INFO("Preparing for GPU mipmap generation: width=%u, height=%u, levels=%u, usage=0x%x",
                            imageCI.extent.width, imageCI.extent.height, imageCI.mipLevels,
                            static_cast<uint32_t>(imageCI.usage));
        }

        auto imageResult = pDevice->create(imageCI, info.debugName);
        if (!imageResult)
        {
            pDevice->destroy(stagingBuffer);
            m_imageAssetPool.free(pImageAsset);
            return {Result::RuntimeError, "Failed to create image: " + imageResult.error().message};
        }

        image = imageResult.value();

        // For simple copy operations, use the transfer queue
        pDevice->executeCommand(
            pTransferQueue,
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
                    .imageExtent       = {
                                          .width = pImageData->width, .height = pImageData->height, .depth = pImageData->depth}
                };

                // Copy from staging buffer to image
                cmd->copy(stagingBuffer, image, {region});

                // Transition to ShaderResource for sampling (if mipmaps aren't being generated)
                if (pImageData->mipLevels.size() == 1 &&
                    (info.featureFlags & ImageFeatureBits::eGenerateMips) == ImageFeatureBits::eNone)
                {
                    barrier.currentState = ResourceState::CopyDest;
                    barrier.newState     = ResourceState::ShaderResource;
                    cmd->insertBarrier({barrier});
                }
            });

        // Check if we need to generate mipmaps using the GPU
        bool gpuMipmapsGenerated = false;
        if (pImageData->mipLevels.size() == 1 &&
            (info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
        {
            // Determine preferred mipmap generation mode
            MipmapGenerationMode mode = MipmapGenerationMode::ePreferGPU;

            // Check if CPU mipmaps are explicitly requested
            if (info.featureFlags & ImageFeatureBits::eForceCPUMipmaps)
                mode = MipmapGenerationMode::eForceCPU;

            // Set filter mode (default to Linear for best quality/performance ratio)
            Filter filterMode = Filter::Linear;

            // Try GPU mipmap generation with the specified mode and filter
            auto genResult = generateMipmapsGPU(pDevice, pGraphicsQueue, image, pImageData->width, pImageData->height,
                                                createInfo.mipLevels, filterMode, mode);

            if (genResult)
            {
                gpuMipmapsGenerated = true;
                LOADER_LOG_INFO("Successfully generated mipmaps using GPU for %s", info.debugName.c_str());
            }
            else if (mode != MipmapGenerationMode::eForceCPU)
            {
                // If GPU generation failed but we're not forcing CPU, try CPU fallback
                LOADER_LOG_WARN("GPU mipmap generation failed: %s. Falling back to CPU.",
                                genResult.error().message.c_str());

                // Generate CPU mipmaps for the ImageData
                auto cpuGenResult = generateMipmaps(pImageData);
                if (cpuGenResult)
                {
                    // Now we have CPU-generated mipmaps, continue with uploading them
                    LOADER_LOG_INFO("Successfully generated mipmaps using CPU for %s", info.debugName.c_str());
                }
                else
                {
                    LOADER_LOG_ERR("CPU mipmap generation also failed: %s", cpuGenResult.error().message.c_str());
                }
            }
            else
            {
                LOADER_LOG_ERR("Mipmap generation failed: %s", genResult.error().message.c_str());
            }
        }
        else if (pImageData->mipLevels.size() > 1 && !gpuMipmapsGenerated)
        {
            // If we have multiple mip levels from CPU generation, upload them
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
                    continue; // Skip this mip level if we can't create a staging buffer
                }

                vk::Buffer* mipStagingBuffer = mipStagingResult.value();

                // Map and copy data to staging buffer
                void* pMapped = pDevice->mapMemory(mipStagingBuffer);
                if (!pMapped)
                {
                    pDevice->destroy(mipStagingBuffer);
                    continue;
                }

                std::memcpy(pMapped, pImageData->mipLevels[i].data.data(), mipDataSize);
                pDevice->unMapMemory(mipStagingBuffer);

                // Copy from staging buffer to image for this mip level
                pDevice->executeCommand(
                    pTransferQueue,
                    [&](auto* cmd)
                    {
                        // Create BufferImageCopy info for this mip level
                        BufferImageCopy region{
                            .bufferOffset      = 0,
                            .bufferRowLength   = 0, // Tightly packed
                            .bufferImageHeight = 0, // Tightly packed
                            .imageSubresource  = {.aspectMask = 1, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1},
                            .imageOffset       = {}, // Zero offset
                            .imageExtent       = {.width  = std::max(1u, pImageData->width >> i),
                                                  .height = std::max(1u, pImageData->height >> i),
                                                  .depth  = pImageData->depth}
                        };

                        // Transition mip level from Undefined/General to CopyDst
                        vk::ImageBarrier barrier{.pImage             = image,
                                                 .currentState       = ResourceState::Undefined,
                                                 .newState           = ResourceState::CopyDest,
                                                 .queueType          = pTransferQueue->getType(),
                                                 .subresourceBarrier = 1, // Only this mip level
                                                 .mipLevel           = static_cast<uint8_t>(i)};
                        cmd->insertBarrier({barrier});

                        // Copy from staging buffer to image
                        cmd->copy(mipStagingBuffer, image, {region});

                        // Transition to ShaderResource for sampling
                        barrier.currentState = ResourceState::CopyDest;
                        barrier.newState     = ResourceState::ShaderResource;
                        cmd->insertBarrier({barrier});
                    });

                // Destroy the staging buffer for this mip level
                pDevice->destroy(mipStagingBuffer);
            }
        }
        else
        {
            // Simple final transition for base mip level since it's the only one we uploaded
            // and no mipmaps were generated. Only needed if we haven't already done this
            // transition during the initial upload.
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
    stagingBuffer = nullptr;

    // Set the image in the asset
    pImageAsset->setImageResource(image);

    return pImageAsset;
}

// Process KTX2 format with feature analysis
Expected<ImageData*> ImageLoader::processKTX2Source(const std::string& path, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Check if we should force reload
    bool forceReload = (info.featureFlags & ImageFeatureBits::eForceReload) != ImageFeatureBits::eNone;
    bool skipCache   = info.forceUncached;

    if (skipCache)
    {
        LOADER_LOG_INFO("Skipping image cache due to forceUncached flag: %s", path.c_str());
    }
    else if (forceReload)
    {
        LOADER_LOG_INFO("Skipping image cache due to ForceReload flag: %s", path.c_str());
    }

    if (!forceReload && !skipCache)
    {
        // Try to load from cache first
        auto cacheKey         = m_imageCache.generateCacheKey(info);
        std::string cachePath = m_imageCache.getCacheFilePath(cacheKey);

        // Verify that the cache file actually exists
        auto& fs         = APH_DEFAULT_FILESYSTEM;
        bool cacheExists = fs.exist(cachePath);

        if (cacheExists || m_imageCache.existsInFileCache(cacheKey))
        {
            LOADER_LOG_INFO("Loading KTX2 texture from cache: %s", cachePath.c_str());
            return loadFromCache(cacheKey);
        }
        else
        {
            LOADER_LOG_INFO("Cache miss for KTX2 texture: %s", cachePath.c_str());
        }
    }

    // Create KTX texture from file
    ktxTexture2* texture = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (result != KTX_SUCCESS)
    {
        return {convertKtxResult(result, "Failed to load KTX2 file: " + path)};
    }

    // Ensure proper cleanup in case of early return
    auto textureGuard =
        std::unique_ptr<ktxTexture2, std::function<void(ktxTexture2*)>>(texture,
                                                                        [](ktxTexture2* p)
                                                                        {
                                                                            if (p)
                                                                                ktxTexture_Destroy(ktxTexture(p));
                                                                        });

    // Analyze KTX2 features
    bool hasMipmaps         = texture->numLevels > 1;
    bool needsTranscoding   = ktxTexture2_NeedsTranscoding(texture);
    bool isFormatCompatible = true; // Default to true, we'll check device compatibility later if needed

    LOADER_LOG_INFO("KTX2 texture %s: mipmaps=%s, basis=%s", path.c_str(), hasMipmaps ? "yes" : "no",
                    needsTranscoding ? "yes" : "no");

    // Direct load case - already has mipmaps and either doesn't need transcoding or format is compatible
    if (hasMipmaps && (!needsTranscoding || isFormatCompatible))
    {
        LOADER_LOG_INFO("Using KTX2 texture directly (optimal format): %s", path.c_str());
        return processKtxTexture2(texture, (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
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
            return {convertKtxResult(result, "Failed to transcode KTX2 texture")};
        }
    }

    // Generate mipmaps if needed
    if (!hasMipmaps && (info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
    {
        // Process the texture to our format first
        auto imageData =
            processKtxTexture2(texture, (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
        if (!imageData)
        {
            return imageData;
        }

        // For KTX2 textures, we'll prefer GPU mipmap generation during final resource creation
        // Only generate CPU mipmaps if explicitly requested or for caching purposes
        bool needsCpuMipmaps = false;

        // Check for explicit CPU mipmap flags
        if (info.featureFlags & ImageFeatureBits::eForceCPUMipmaps)
            needsCpuMipmaps = true;

        // Generate CPU mipmaps for caching purposes
        if (needsCpuMipmaps)
        {
            // Generate mipmaps on CPU
            auto mipmappedResult = generateMipmaps(imageData.value());
            if (!mipmappedResult)
            {
                m_imageDataPool.free(imageData.value());
                return mipmappedResult.transform([](bool) { return nullptr; });
            }
        }

        // Cache the enhanced version (either with CPU mipmaps or just base level that will get GPU mipmaps)
        auto cacheKey         = m_imageCache.generateCacheKey(info);
        std::string cachePath = m_imageCache.getCacheFilePath(cacheKey);

        // Double-check if it's already cached to avoid redundant writes
        auto& fs = APH_DEFAULT_FILESYSTEM;
        if (!fs.exist(cachePath))
        {
            auto cacheResult = encodeToCacheFile(imageData.value(), cachePath);
            if (!cacheResult)
            {
                LOADER_LOG_WARN("Failed to cache enhanced KTX2 texture: %s", path.c_str());
                // Continue anyway - caching failure is not fatal
            }
            else
            {
                LOADER_LOG_INFO("*** CACHE CREATED *** KTX2 texture: %s", cachePath.c_str());

                // Update cache info in the image data
                imageData.value()->isCached  = true;
                imageData.value()->cacheKey  = cacheKey;
                imageData.value()->cachePath = cachePath;

                // Add to the memory cache
                m_imageCache.addImage(cacheKey, imageData.value());
            }
        }
        else
        {
            LOADER_LOG_INFO("Using existing cached KTX2 texture: %s", cachePath.c_str());
        }

        return imageData;
    }

    // If we reached here, just return the processed texture
    return processKtxTexture2(texture, (info.featureFlags & ImageFeatureBits::eFlipY) != ImageFeatureBits::eNone);
}

// Process standard image formats with caching
Expected<ImageData*> ImageLoader::processStandardFormat(const std::string& resolvedPath, const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    // Check if we should force reload
    bool forceReload = (info.featureFlags & ImageFeatureBits::eForceReload) != ImageFeatureBits::eNone;
    bool skipCache   = info.forceUncached;

    if (skipCache)
    {
        LOADER_LOG_INFO("Skipping image cache due to forceUncached flag: %s", resolvedPath.c_str());
    }
    else if (forceReload)
    {
        LOADER_LOG_INFO("Skipping image cache due to ForceReload flag: %s", resolvedPath.c_str());
    }

    if (!forceReload && !skipCache)
    {
        // Try to load from cache first
        auto cacheKey         = m_imageCache.generateCacheKey(info);
        std::string cachePath = m_imageCache.getCacheFilePath(cacheKey);

        // Verify that the cache file actually exists
        auto& fs         = APH_DEFAULT_FILESYSTEM;
        bool cacheExists = fs.exist(cachePath);

        if (cacheExists || m_imageCache.existsInFileCache(cacheKey))
        {
            LOADER_LOG_INFO("Loading from cache: %s", cachePath.c_str());

            // Return the cached image data
            return loadFromCache(cacheKey);
        }
        else
        {
            LOADER_LOG_INFO("Cache miss for: %s", cachePath.c_str());
        }
    }

    // Determine file type and load appropriate format
    ImageContainerType containerType     = aph::detectFileType(resolvedPath);
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
        return {Result::RuntimeError, "Unsupported image format"};
    }

    // Check if loading was successful
    if (!imageDataResult)
    {
        return imageDataResult;
    }

    // Generate mipmaps if requested
    if ((info.featureFlags & ImageFeatureBits::eGenerateMips) != ImageFeatureBits::eNone)
    {
        // For standard format textures, we'll prefer GPU mipmap generation during final resource creation
        // Only generate CPU mipmaps if explicitly requested or for caching purposes
        bool needsCpuMipmaps = false;

        // Check for explicit CPU mipmap flags
        if (info.featureFlags & ImageFeatureBits::eForceCPUMipmaps)
            needsCpuMipmaps = true;

        // Generate CPU mipmaps for caching purposes
        if (needsCpuMipmaps)
        {
            auto genResult = generateMipmaps(imageDataResult.value());
            if (!genResult)
            {
                m_imageDataPool.free(imageDataResult.value());
                return genResult.transform([](bool) { return nullptr; });
            }
        }
    }

    // Cache the result if caching is enabled
    if (!forceReload)
    {
        auto cacheKey         = m_imageCache.generateCacheKey(info);
        std::string cachePath = m_imageCache.getCacheFilePath(cacheKey);

        auto cacheResult = encodeToCacheFile(imageDataResult.value(), cachePath);
        if (!cacheResult)
        {
            LOADER_LOG_WARN("Failed to cache texture: %s", cachePath.c_str());
            // Continue anyway - caching failure is not fatal
        }
        else
        {
            LOADER_LOG_INFO("*** CACHE CREATED *** texture: %s", cachePath.c_str());

            // Update cache info in the image data
            imageDataResult.value()->isCached  = true;
            imageDataResult.value()->cacheKey  = cacheKey;
            imageDataResult.value()->cachePath = cachePath;

            // Add to the memory cache
            m_imageCache.addImage(cacheKey, imageDataResult.value());
        }
    }

    return imageDataResult;
}
} // namespace aph
