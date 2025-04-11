#pragma once

#include "imageAsset.h"
#include "common/result.h"
#include <ktx.h>
#include <ktxvulkan.h>
#include <variant>

namespace aph
{
// Format conversion helpers
ImageFormat getFormatFromChannels(int channels);
ImageFormat getFormatFromVulkan(VkFormat vkFormat);
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI);
ImageContainerType detectFileType(const std::string& path);

// KTX utility functions
Result convertKtxResult(KTX_error_code ktxResult, const std::string& operation = "");
using KtxTextureVariant = std::variant<ktxTexture*, ktxTexture2*>;
Expected<ImageMipLevel> fillMipLevel(const KtxTextureVariant& textureVar, 
                                     uint32_t level, bool isFlipY, uint32_t width, uint32_t height);

// Mipmap generation utility
Expected<bool> generateMipmaps(ImageData* pImageData);

// Cache utilities
Expected<bool> encodeToCacheFile(ImageData* pImageData, const std::string& cachePath);

} // namespace aph
