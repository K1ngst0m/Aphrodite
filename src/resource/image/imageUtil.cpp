#include "imageUtil.h"
#include "api/vulkan/device.h"
#include "common/profiler.h"

namespace aph
{

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

Expected<ImageMipLevel> fillMipLevel(const KtxTextureVariant& textureVar, uint32_t level, bool isFlipY, uint32_t width,
                                     uint32_t height)
{
    APH_PROFILER_SCOPE();

    // Create and initialize the mip level structure
    ImageMipLevel mipLevel;

    // Calculate dimensions for this mip level
    mipLevel.width  = std::max(1U, width >> level);
    mipLevel.height = std::max(1U, height >> level);

    // Get the image size, offset, and data based on the texture type
    ktx_size_t levelSize  = 0;
    ktx_size_t offset     = 0;
    KTX_error_code result = KTX_SUCCESS;
    uint8_t* levelData    = nullptr;

    // Process based on texture type
    if (std::holds_alternative<ktxTexture*>(textureVar))
    {
        ktxTexture* texture = std::get<ktxTexture*>(textureVar);

        // Get the image size
        levelSize = ktxTexture_GetImageSize(texture, level);

        // Get image data offset
        result = ktxTexture_GetImageOffset(texture, level, 0, 0, &offset);
        if (result != KTX_SUCCESS)
        {
            return {convertKtxResult(result, "Failed to get image offset for level " + std::to_string(level))};
        }

        // Get pointer to image data
        levelData = ktxTexture_GetData(texture) + offset;
    }
    else if (std::holds_alternative<ktxTexture2*>(textureVar))
    {
        ktxTexture2* texture = std::get<ktxTexture2*>(textureVar);

        // Cast to base texture type for API compatibility
        ktxTexture* baseTexture = ktxTexture(texture);

        // Get the image size
        levelSize = ktxTexture_GetImageSize(baseTexture, level);

        // Get image data offset
        result = ktxTexture_GetImageOffset(baseTexture, level, 0, 0, &offset);
        if (result != KTX_SUCCESS)
        {
            return {convertKtxResult(result, "Failed to get image offset for level " + std::to_string(level))};
        }

        // Get pointer to image data
        levelData = ktxTexture_GetData(baseTexture) + offset;

        // Special case: don't flip compressed textures in KTX2
        if (isFlipY && ktxTexture2_NeedsTranscoding(texture))
        {
            isFlipY = false;
        }
    }
    else
    {
        return {Result::RuntimeError, "Invalid texture variant type"};
    }

    // Calculate row pitch (may need adjustment based on the format)
    mipLevel.rowPitch = mipLevel.width * 4; // Assuming RGBA8 for now

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
            std::memcpy(flippedData.data() + y * mipLevel.rowPitch, mipLevel.data.data() + srcRow * mipLevel.rowPitch,
                        mipLevel.rowPitch);
        }

        // Swap with the flipped data
        mipLevel.data.swap(flippedData);
    }

    return mipLevel;
}

ImageContainerType detectFileType(const std::string& path)
{
    APH_PROFILER_SCOPE();

    // Handle potential exception if path doesn't contain a dot
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return ImageContainerType::eDefault;
    }

    const std::string extension = path.substr(dotPos);

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

Expected<bool> generateMipmaps(ImageData* pImageData)
{
    APH_PROFILER_SCOPE();

    // Skip if no image data or already has mipmaps
    if (!pImageData || pImageData->mipLevels.size() > 1)
    {
        return true;
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
        return true;
    }

    // Check if we have the first mip level
    if (pImageData->mipLevels.empty())
    {
        return {Result::RuntimeError, "Cannot generate mipmaps: base level missing"};
    }

    // Ensure the base level is RGBA8 format (for simplicity)
    if (pImageData->format != ImageFormat::eR8G8B8A8Unorm)
    {
        return {Result::RuntimeError, "Mipmap generation only supported for RGBA8 images"};
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

    return true;
}

Expected<bool> encodeToCacheFile(ImageData* pImageData, const std::string& cachePath)
{
    APH_PROFILER_SCOPE();

    if (!pImageData || pImageData->mipLevels.empty())
    {
        return {Result::RuntimeError, "Invalid image data for caching"};
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
    ktxTextureCreateInfo createInfo = {
        .glInternalformat = 0, // Ignored for KTX2
        .vkFormat         = VK_FORMAT_R8G8B8A8_UNORM, // Default format
        .baseWidth        = pImageData->width,
        .baseHeight       = pImageData->height,
        .baseDepth        = pImageData->depth,
        .numDimensions    = static_cast<ktx_uint32_t>((pImageData->depth > 1) ? 3 : 2),
        .numLevels        = static_cast<uint32_t>(pImageData->mipLevels.size()),
        .numLayers        = pImageData->arraySize,
        .numFaces         = 1,
        .isArray          = pImageData->arraySize > 1 ? KTX_TRUE : KTX_FALSE,
        .generateMipmaps  = KTX_FALSE // We already have mip levels
    };

    // Create the KTX2 texture
    KTX_error_code result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);

    if (result != KTX_SUCCESS)
    {
        return {convertKtxResult(result, "Failed to create KTX2 texture for encoding")};
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
            return {convertKtxResult(result, "Failed to set image data for mip level " + std::to_string(level))};
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
            return {convertKtxResult(result, "Failed to compress texture with Basis Universal")};
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
        return {convertKtxResult(result, "Failed to write KTX2 file: " + cachePath)};
    }

    // Record the time encoded
    pImageData->timeEncoded = std::chrono::steady_clock::now().time_since_epoch().count();

    return {true};
}

Expected<bool> generateMipmapsGPU(vk::Device* pDevice, vk::Queue* pQueue, vk::Image* pImage, uint32_t width,
                                  uint32_t height, uint32_t mipLevels, Filter filterMode, MipmapGenerationMode mode)
{
    APH_PROFILER_SCOPE();

    // Validate input parameters
    if (!pDevice || !pQueue || !pImage)
    {
        return {Result::RuntimeError, "Invalid device, queue, or image pointer"};
    }

    // Skip if we only have one mip level
    if (mipLevels <= 1)
    {
        return true; // Nothing to do
    }

    // Check if the image has the appropriate usage flags for blit operations
    ImageUsageFlags imageUsage = pImage->getCreateInfo().usage;
    bool canUseGPU             = ((imageUsage & ImageUsage::TransferSrc) != ImageUsage::None) &&
                     ((imageUsage & ImageUsage::TransferDst) != ImageUsage::None);

    // If we can't use GPU and force GPU was specified, return an error
    if (!canUseGPU && mode == MipmapGenerationMode::eForceGPU)
    {
        return {Result::RuntimeError, "Image doesn't have required usage flags for GPU mipmap generation"};
    }

    // Fall back to CPU if required
    if (!canUseGPU && mode == MipmapGenerationMode::ePreferGPU)
    {
        CM_LOG_WARN("GPU mipmap generation not possible, falling back to CPU");
        return {Result::RuntimeError, "GPU mipmap generation not possible, caller should use CPU implementation"};
    }

    // Create a command to generate the mipmaps
    pDevice->executeCommand(
        pQueue,
        [&](vk::CommandBuffer* cmd)
        {
            // First transition the base level to TransferSrc
            vk::ImageBarrier barrierBaseLevel{.pImage       = pImage,
                                              .currentState = ResourceState::CopyDest, // Coming from the initial upload
                                              .newState     = ResourceState::CopySource,
                                              .queueType    = pQueue->getType(),
                                              .subresourceBarrier = 1, // Target only base level
                                              .mipLevel           = 0};
            cmd->insertBarrier({barrierBaseLevel});

            // Previous width and height for each iteration
            auto mipWidth  = static_cast<int32_t>(width);
            auto mipHeight = static_cast<int32_t>(height);

            // Generate each mip level using vk::CommandBuffer::blit
            for (uint32_t i = 1; i < mipLevels; i++)
            {
                // Calculate mip dimensions for this level
                int32_t nextMipWidth  = std::max(1, mipWidth / 2);
                int32_t nextMipHeight = std::max(1, mipHeight / 2);

                // Transition the mip level to TransferDst
                vk::ImageBarrier barrierMipDst{.pImage       = pImage,
                                               .currentState = ResourceState::Undefined, // First use of this mip level
                                               .newState     = ResourceState::CopyDest,
                                               .queueType    = pQueue->getType(),
                                               .subresourceBarrier = 1, // Target only this mip level
                                               .mipLevel           = static_cast<uint8_t>(i)};
                cmd->insertBarrier({barrierMipDst});

                // Create source and destination blit info
                vk::ImageBlitInfo srcBlitInfo{
                    .offset     = {       .x = 0,         .y = 0, .z = 0},
                    .extent     = {.x = mipWidth, .y = mipHeight, .z = 1},
                    .level      = i - 1,
                    .baseLayer  = 0,
                    .layerCount = 1
                };

                vk::ImageBlitInfo dstBlitInfo{
                    .offset     = {           .x = 0,             .y = 0, .z = 0},
                    .extent     = {.x = nextMipWidth, .y = nextMipHeight, .z = 1},
                    .level      = i,
                    .baseLayer  = 0,
                    .layerCount = 1
                };

                // Perform the blit with specified filter mode
                cmd->blit(pImage, pImage, srcBlitInfo, dstBlitInfo, filterMode);

                // Transition the mip level i-1 from TRANSFER_SRC to SHADER_RESOURCE
                vk::ImageBarrier barrierPrevToSR{.pImage             = pImage,
                                                 .currentState       = ResourceState::CopySource,
                                                 .newState           = ResourceState::ShaderResource,
                                                 .queueType          = pQueue->getType(),
                                                 .subresourceBarrier = 1, // Target only previous level
                                                 .mipLevel           = static_cast<uint8_t>(i - 1)};
                cmd->insertBarrier({barrierPrevToSR});

                // Transition the current mip level from TRANSFER_DST to TRANSFER_SRC for next iteration
                if (i < mipLevels - 1)
                {
                    vk::ImageBarrier barrierToSrc{.pImage             = pImage,
                                                  .currentState       = ResourceState::CopyDest,
                                                  .newState           = ResourceState::CopySource,
                                                  .queueType          = pQueue->getType(),
                                                  .subresourceBarrier = 1, // Target only this level
                                                  .mipLevel           = static_cast<uint8_t>(i)};
                    cmd->insertBarrier({barrierToSrc});
                }
                else
                {
                    // Last mip level, transition to SHADER_RESOURCE
                    vk::ImageBarrier barrierLastToSR{.pImage             = pImage,
                                                     .currentState       = ResourceState::CopyDest,
                                                     .newState           = ResourceState::ShaderResource,
                                                     .queueType          = pQueue->getType(),
                                                     .subresourceBarrier = 1, // Target only last level
                                                     .mipLevel           = static_cast<uint8_t>(i)};
                    cmd->insertBarrier({barrierLastToSR});
                }

                // Update dimensions for next iteration
                mipWidth  = nextMipWidth;
                mipHeight = nextMipHeight;
            }
        });

    return true;
}

} // namespace aph
