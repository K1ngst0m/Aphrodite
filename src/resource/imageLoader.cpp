#include "imageLoader.h"

#include "common/common.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "resourceLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace aph
{
std::shared_ptr<ImageData> loadImageFromFile(std::string_view path, bool isFlipY = false);
std::array<std::shared_ptr<ImageData>, 6> loadCubemapFromFiles(const std::array<std::string_view, 6>& paths);
bool loadKTX(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data);
bool loadPNGJPG(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data,
                bool isFlipY = false);

ImageFormat getFormatFromChannels(int channels);
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI);
} // namespace aph

namespace aph
{
//-----------------------------------------------------------------------------
// ImageAsset Implementation
//-----------------------------------------------------------------------------

ImageAsset::ImageAsset()
    : m_pImageResource(nullptr)
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

void ImageAsset::setImageResource(vk::Image* pImage)
{
    m_pImageResource = pImage;
}

//-----------------------------------------------------------------------------
// ImageLoader Implementation
//-----------------------------------------------------------------------------

ImageLoader::ImageLoader(ResourceLoader* pResourceLoader)
    : m_pResourceLoader(pResourceLoader)
{
}

ImageLoader::~ImageLoader()
{
}

Result ImageLoader::loadFromFile(const ImageLoadInfo& info, ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Create the asset
    *ppImageAsset = m_imageAssetPools.allocate();

    // Check if we're loading raw data instead of a file
    if (std::holds_alternative<ImageInfo>(info.data))
    {
        return loadRawData(info, ppImageAsset);
    }

    // Get the file path
    auto& pathStr = std::get<std::string>(info.data);
    auto path = std::filesystem::path{APH_DEFAULT_FILESYSTEM.resolvePath(pathStr)};
    auto ext = path.extension();

    // If container type was specified, use it; otherwise determine from extension
    ImageContainerType containerType = info.containerType;
    if (containerType == ImageContainerType::Default)
    {
        containerType = GetImageContainerType(path);
        if (containerType == ImageContainerType::Default)
        {
            destroy(*ppImageAsset);
            *ppImageAsset = nullptr;
            return {Result::RuntimeError, "Unsupported image file format: " + std::string(ext.c_str())};
        }
    }

    // Special case for cubemaps
    if (info.featureFlags & ImageFeatureBits::Cubemap)
    {
        // This would extract the base path and append _posx, _negx, etc.
        // For now, just return an error
        destroy(*ppImageAsset);
        *ppImageAsset = nullptr;
        return {Result::RuntimeError, "Cubemap loading not implemented yet"};
    }

    // Handle different file formats
    switch (containerType)
    {
    case ImageContainerType::Ktx:
        return loadKTX(info, ppImageAsset);
    case ImageContainerType::Png:
        return loadPNG(info, ppImageAsset);
    case ImageContainerType::Jpg:
        return loadJPG(info, ppImageAsset);
    default:
        destroy(*ppImageAsset);
        *ppImageAsset = nullptr;
        return {Result::RuntimeError, "Unsupported image container type"};
    }
}

void ImageLoader::destroy(ImageAsset* pImageAsset)
{
    if (pImageAsset)
    {
        // Destroy the underlying image resource
        vk::Image* pImage = pImageAsset->getImage();
        if (pImage)
        {
            m_pResourceLoader->getDevice()->destroy(pImage);
        }

        // Delete the asset
        m_imageAssetPools.free(pImageAsset);
    }
}

Result ImageLoader::loadPNG(const ImageLoadInfo& info, ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Get file path
    auto& pathStr = std::get<std::string>(info.data);
    std::filesystem::path path = APH_DEFAULT_FILESYSTEM.resolvePath(pathStr);

    // Load image data
    bool isFlipY = (info.featureFlags & ImageFeatureBits::FlipY) != ImageFeatureBits::None;
    std::shared_ptr<ImageData> imageData = loadImageFromFile(path.string(), isFlipY);

    if (!imageData || imageData->mipLevels.empty())
    {
        destroy(*ppImageAsset);
        *ppImageAsset = nullptr;
        return {Result::RuntimeError, "Failed to load PNG image: " + path.string()};
    }

    // Create image resource from loaded data
    return createImageResources(imageData, info, ppImageAsset);
}

Result ImageLoader::loadJPG(const ImageLoadInfo& info, ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Get file path
    auto& pathStr = std::get<std::string>(info.data);
    std::filesystem::path path = APH_DEFAULT_FILESYSTEM.resolvePath(pathStr);

    // Load image data
    bool isFlipY = (info.featureFlags & ImageFeatureBits::FlipY) != ImageFeatureBits::None;
    std::shared_ptr<ImageData> imageData = loadImageFromFile(path.string(), isFlipY);

    if (!imageData || imageData->mipLevels.empty())
    {
        destroy(*ppImageAsset);
        *ppImageAsset = nullptr;
        return {Result::RuntimeError, "Failed to load JPG image: " + path.string()};
    }

    // Create image resource from loaded data
    return createImageResources(imageData, info, ppImageAsset);
}

Result ImageLoader::loadKTX(const ImageLoadInfo& info, ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // KTX loading is not yet implemented
    CM_LOG_ERR("KTX loading not implemented yet");
    destroy(*ppImageAsset);
    *ppImageAsset = nullptr;
    return {Result::RuntimeError, "KTX loading not implemented yet"};
}

Result ImageLoader::loadRawData(const ImageLoadInfo& info, ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Get the raw image data
    auto& imageInfo = std::get<ImageInfo>(info.data);

    // Create a new ImageData
    auto imageData = std::make_shared<ImageData>();
    imageData->width = imageInfo.width;
    imageData->height = imageInfo.height;
    imageData->depth = 1;
    imageData->arraySize = 1;
    imageData->format = ImageFormat::R8G8B8A8_UNORM; // Assume RGBA format for raw data

    // Create single mip level
    ImageMipLevel mipLevel;
    mipLevel.width = imageInfo.width;
    mipLevel.height = imageInfo.height;
    mipLevel.rowPitch = imageInfo.width * 4; // 4 bytes per pixel for RGBA
    mipLevel.data = imageInfo.data;

    imageData->mipLevels.push_back(std::move(mipLevel));

    // Create image resource from the data
    return createImageResources(imageData, info, ppImageAsset);
}

Result ImageLoader::loadCubemap(const std::array<std::string, 6>& paths, const ImageLoadInfo& info,
                                ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Convert paths to string_view
    std::array<std::string_view, 6> pathViews;
    for (size_t i = 0; i < 6; i++)
    {
        pathViews[i] = paths[i];
    }

    // Load cubemap images
    auto cubemapImages = loadCubemapFromFiles(pathViews);

    // Verify all faces loaded successfully
    for (const auto& image : cubemapImages)
    {
        if (!image || image->mipLevels.empty())
        {
            destroy(*ppImageAsset);
            *ppImageAsset = nullptr;
            return {Result::RuntimeError, "Failed to load one or more cubemap faces"};
        }
    }

    // For now, return an error since cubemap resource creation is not implemented
    destroy(*ppImageAsset);
    *ppImageAsset = nullptr;
    return {Result::RuntimeError, "Cubemap resource creation not implemented yet"};
}

Result ImageLoader::createImageResources(std::shared_ptr<ImageData> imageData, const ImageLoadInfo& info,
                                         ImageAsset** ppImageAsset)
{
    APH_PROFILER_SCOPE();

    // Create ImageCreateInfo from the loaded data
    vk::ImageCreateInfo createInfo = info.createInfo;

    // If not already specified, set the create info from image data
    if (createInfo.extent.width == 0 || createInfo.extent.height == 0)
    {
        convertToVulkanFormat(*imageData, createInfo);
    }

    // Set mip levels based on whether we're generating mips
    bool generateMips = (info.featureFlags & ImageFeatureBits::GenerateMips) != ImageFeatureBits::None;
    if (generateMips)
    {
        // Calculate max possible mip levels
        uint32_t maxDimension = std::max(imageData->width, imageData->height);
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;
        createInfo.mipLevels = mipLevels;
    }
    else
    {
        createInfo.mipLevels = static_cast<uint32_t>(imageData->mipLevels.size());
    }

    // Set default usage flags if not already specified
    if (createInfo.usage == ImageUsage::None)
    {
        if (generateMips)
        {
            createInfo.usage = ImageUsage::Sampled | ImageUsage::TransferDst | ImageUsage::TransferSrc;
        }
        else
        {
            createInfo.usage = ImageUsage::Sampled | ImageUsage::TransferDst;
        }
    }
    else if (generateMips && !(createInfo.usage & ImageUsage::TransferSrc))
    {
        // Ensure TransferSrc is set if we're generating mipmaps
        createInfo.usage |= ImageUsage::TransferSrc;
    }

    // Access the device and queues
    vk::Device* pDevice = m_pResourceLoader->getDevice();
    vk::Queue* pTransferQueue = pDevice->getQueue(QueueType::Transfer);
    vk::Queue* pGraphicsQueue = pDevice->getQueue(QueueType::Graphics);

    // Get the data from the first mip level
    std::vector<uint8_t>& data = imageData->mipLevels[0].data;

    // Create a staging buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size = data.size(),
            .usage = BufferUsage::TransferSrc,
            .domain = MemoryDomain::Upload,
        };
        auto stagingResult = pDevice->create(bufferCI, std::string{info.debugName} + std::string{"_staging"});
        APH_VERIFY_RESULT(stagingResult);
        stagingBuffer = stagingResult.value();

        // Map and copy data to staging buffer
        void* pMapped = pDevice->mapMemory(stagingBuffer);
        APH_ASSERT(pMapped);
        std::memcpy(pMapped, data.data(), data.size());
        pDevice->unMapMemory(stagingBuffer);
    }

    // Create the image
    vk::Image* image;
    {
        auto imageCI = createInfo;
        // Ensure we're not losing any usage flags
        if (generateMips)
        {
            imageCI.usage |= (ImageUsage::TransferDst | ImageUsage::TransferSrc);
        }
        else
        {
            imageCI.usage |= ImageUsage::TransferDst;
        }
        imageCI.domain = MemoryDomain::Device;

        auto imageResult = pDevice->create(imageCI, info.debugName);
        APH_VERIFY_RESULT(imageResult);
        image = imageResult.value();

        // For simple copy operations, use the transfer queue
        pDevice->executeCommand(pTransferQueue,
                                [&](auto* cmd)
                                {
                                    cmd->transitionImageLayout(image, ResourceState::Undefined,
                                                               ResourceState::CopyDest);
                                    cmd->copy(stagingBuffer, image);
                                });

        // For mipmap generation that requires blitting, use the graphics queue
        if (generateMips)
        {
            pDevice->executeCommand(
                pGraphicsQueue,
                [&](auto* cmd)
                {
                    // First transition the image to source layout for the first blit operation
                    vk::ImageBarrier initialBarrier{
                        .pImage = image,
                        .currentState = ResourceState::CopyDest,
                        .newState = ResourceState::CopySource,
                        .subresourceBarrier = 1,
                        .mipLevel = 0, // Base mip level will be the source
                    };
                    cmd->insertBarrier({initialBarrier});

                    uint32_t width = createInfo.extent.width;
                    uint32_t height = createInfo.extent.height;

                    // generate mipmap chains
                    for (uint32_t i = 1; i < imageCI.mipLevels; i++)
                    {
                        vk::ImageBlitInfo srcBlitInfo{
                            .extent = {(int32_t)(width >> (i - 1)), (int32_t)(height >> (i - 1)), 1},
                            .level = i - 1,
                            .layerCount = 1,
                        };

                        vk::ImageBlitInfo dstBlitInfo{
                            .extent = {(int32_t)(width >> i), (int32_t)(height >> i), 1},
                            .level = i,
                            .layerCount = 1,
                        };

                        // Prepare current mip level as image blit destination
                        vk::ImageBarrier toDestBarrier{
                            .pImage = image,
                            .currentState =
                                ResourceState::Undefined, // First use of each new mip level starts as Undefined
                            .newState = ResourceState::CopyDest,
                            .subresourceBarrier = 1,
                            .mipLevel = static_cast<uint8_t>(i),
                        };
                        cmd->insertBarrier({toDestBarrier});

                        // Blit from previous level
                        cmd->blit(image, image, srcBlitInfo, dstBlitInfo);

                        // After blitting, transition current level to CopySource for next iteration
                        vk::ImageBarrier toSrcBarrier{
                            .pImage = image,
                            .currentState = ResourceState::CopyDest,
                            .newState = ResourceState::CopySource,
                            .subresourceBarrier = 1,
                            .mipLevel = static_cast<uint8_t>(i),
                        };
                        cmd->insertBarrier({toSrcBarrier});
                    }

                    // Final transition to ShaderResource - for all mip levels
                    vk::ImageBarrier finalBarrier{
                        .pImage = image,
                        .currentState = ResourceState::CopySource,
                        .newState = ResourceState::ShaderResource,
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
                                            .pImage = image,
                                            .currentState = ResourceState::CopyDest,
                                            .newState = ResourceState::ShaderResource,
                                            .subresourceBarrier = 0, // 0 means apply to all mip levels
                                        };
                                        cmd->insertBarrier({finalBarrier});
                                    });
        }
    }

    // Cleanup staging buffer
    pDevice->destroy(stagingBuffer);

    // Set the image in the asset
    (*ppImageAsset)->setImageResource(image);

    return Result::Success;
}

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

//-----------------------------------------------------------------------------
// ImageLoader helper methods
//-----------------------------------------------------------------------------

ImageContainerType ImageLoader::GetImageContainerType(const std::filesystem::path& path)
{
    APH_PROFILER_SCOPE();
    if (path.extension() == ".ktx")
    {
        return ImageContainerType::Ktx;
    }

    if (path.extension() == ".png")
    {
        return ImageContainerType::Png;
    }

    if (path.extension() == ".jpg" || path.extension() == ".jpeg")
    {
        return ImageContainerType::Jpg;
    }

    CM_LOG_ERR("Unsupported image format: %s", path.c_str());
    return ImageContainerType::Default;
}

//-----------------------------------------------------------------------------
// Utility Functions
//-----------------------------------------------------------------------------

ImageFormat getFormatFromChannels(int channels)
{
    switch (channels)
    {
    case 1:
        return ImageFormat::R8_UNORM;
    case 2:
        return ImageFormat::R8G8_UNORM;
    case 3:
        return ImageFormat::R8G8B8_UNORM;
    case 4:
        return ImageFormat::R8G8B8A8_UNORM;
    default:
        return ImageFormat::Unknown;
    }
}

// Converts our internal format to Vulkan format for API consumption
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI)
{
    APH_PROFILER_SCOPE();

    outCI.extent = {
        .width = imageData.width,
        .height = imageData.height,
        .depth = imageData.depth,
    };

    // Simple format conversion logic - in a real engine you'd have a more complete mapping
    switch (imageData.format)
    {
    case ImageFormat::R8_UNORM:
        outCI.format = aph::Format::R8_UNORM;
        break;
    case ImageFormat::R8G8_UNORM:
        outCI.format = aph::Format::RG8_UNORM;
        break;
    case ImageFormat::R8G8B8_UNORM:
    case ImageFormat::R8G8B8A8_UNORM:
        outCI.format = aph::Format::RGBA8_UNORM;
        break;
    case ImageFormat::BC1_RGB_UNORM:
        outCI.format = aph::Format::BC1_UNORM;
        break;
    case ImageFormat::BC3_RGBA_UNORM:
        outCI.format = aph::Format::BC3_UNORM;
        break;
    case ImageFormat::BC5_RG_UNORM:
        outCI.format = aph::Format::BC5_UNORM;
        break;
    case ImageFormat::BC7_RGBA_UNORM:
        outCI.format = aph::Format::BC7_UNORM;
        break;
    default:
        outCI.format = aph::Format::RGBA8_UNORM;
        break;
    }

    outCI.mipLevels = static_cast<uint32_t>(imageData.mipLevels.size());
    outCI.arraySize = imageData.arraySize;
}

std::shared_ptr<ImageData> loadImageFromFile(std::string_view path, bool isFlipY)
{
    APH_PROFILER_SCOPE();

    // Check if image is already in cache
    auto& cache = ImageCache::get();
    auto cachedImage = cache.findImage(std::string(path));
    if (cachedImage)
    {
        return cachedImage;
    }

    auto image = std::make_shared<ImageData>();
    stbi_set_flip_vertically_on_load(isFlipY);

    int width, height, channels;
    uint8_t* img = stbi_load(path.data(), &width, &height, &channels, 0);
    if (img == nullptr)
    {
        CM_LOG_ERR("Error loading image: %s", path.data());
        return nullptr;
    }

    image->width = width;
    image->height = height;
    image->format = getFormatFromChannels(channels);

    // Create base mip level
    ImageMipLevel baseMip;
    baseMip.width = width;
    baseMip.height = height;
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

    image->mipLevels.push_back(std::move(baseMip));
    stbi_image_free(img);

    // Add to cache
    cache.addImage(std::string(path), image);

    return image;
}

std::array<std::shared_ptr<ImageData>, 6> loadCubemapFromFiles(const std::array<std::string_view, 6>& paths)
{
    APH_PROFILER_SCOPE();
    std::array<std::shared_ptr<ImageData>, 6> cubemapImages;

    for (size_t i = 0; i < 6; i++)
    {
        cubemapImages[i] = loadImageFromFile(paths[i]);

        // Validate that all faces have the same dimensions
        if (i > 0)
        {
            APH_ASSERT(cubemapImages[i]->width == cubemapImages[0]->width);
            APH_ASSERT(cubemapImages[i]->height == cubemapImages[0]->height);
        }
    }

    return cubemapImages;
}

bool loadKTX(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    APH_PROFILER_SCOPE();

    // For now, implement a stub that returns false
    // In a real implementation, this would load KTX files with format and mipmap support
    CM_LOG_ERR("KTX loading not implemented yet: %s", path.c_str());
    APH_ASSERT(false);
    return false;
}

bool loadPNGJPG(const std::filesystem::path& path, vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data, bool isFlipY)
{
    APH_PROFILER_SCOPE();

    auto imageData = loadImageFromFile(path.c_str(), isFlipY);

    if (!imageData || imageData->mipLevels.empty())
    {
        CM_LOG_ERR("Failed to load image: %s", path.c_str());
        return false;
    }

    convertToVulkanFormat(*imageData, outCI);

    // For multiple mip levels, we would need to handle packing here
    // For now we just use the base level
    const auto& baseMip = imageData->mipLevels[0];
    data = baseMip.data;

    return true;
}

} // namespace aph
