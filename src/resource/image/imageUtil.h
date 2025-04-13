#pragma once

#include "common/result.h"
#include "imageAsset.h"
#include <ktx.h>
#include <ktxvulkan.h>
#include <variant>

namespace aph
{
// Forward declarations
namespace vk
{
class Device;
class Queue;
class Image;
class CommandBuffer;
} // namespace vk

// Format conversion helpers
ImageFormat getFormatFromChannels(int channels);
ImageFormat getFormatFromVulkan(VkFormat vkFormat);
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI);
ImageContainerType detectFileType(const std::string& path);

// KTX utility functions
Result convertKtxResult(KTX_error_code ktxResult, const std::string& operation = "");
using KtxTextureVariant = std::variant<ktxTexture*, ktxTexture2*>;
Expected<ImageMipLevel> fillMipLevel(const KtxTextureVariant& textureVar, uint32_t level, bool isFlipY, uint32_t width,
                                     uint32_t height);

// Mipmap generation utility
Expected<bool> generateMipmaps(ImageData* pImageData);

// GPU-based mipmap generation with CPU fallback
enum class MipmapGenerationMode : uint8_t
{
    ePreferGPU, // Use GPU when possible, fall back to CPU
    eForceGPU, // Use GPU only, fail if not possible
    eForceCPU // Always use CPU generation
};

// GPU-based mipmap generation
Expected<bool> generateMipmapsGPU(vk::Device* pDevice, vk::Queue* pQueue, vk::Image* pImage, uint32_t width,
                                  uint32_t height, uint32_t mipLevels, Filter filterMode = Filter::Linear,
                                  MipmapGenerationMode mode = MipmapGenerationMode::ePreferGPU);

// Cache utilities
Expected<bool> encodeToCacheFile(ImageData* pImageData, const std::string& cachePath);

} // namespace aph
