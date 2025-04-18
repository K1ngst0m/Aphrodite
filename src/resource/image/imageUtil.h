#pragma once

#include "common/result.h"
#include "imageAsset.h"
#include "ktx.h"
#include "resource/forward.h"

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
auto getFormatFromChannels(int channels) -> ImageFormat;
auto getFormatFromVulkan(VkFormat vkFormat) -> ImageFormat;
void convertToVulkanFormat(const ImageData& imageData, vk::ImageCreateInfo& outCI);
auto detectFileType(const std::string& path) -> ImageContainerType;

// KTX utility functions
auto convertKtxResult(KTX_error_code ktxResult, const std::string& operation = "") -> Result;
using KtxTextureVariant = std::variant<ktxTexture*, ktxTexture2*>;
auto fillMipLevel(const KtxTextureVariant& textureVar, uint32_t level, bool isFlipY, uint32_t width, uint32_t height)
    -> Expected<ImageMipLevel>;

// Mipmap generation utility
auto generateMipmaps(ImageData* pImageData) -> Expected<bool>;

// GPU-based mipmap generation with CPU fallback
enum class MipmapGenerationMode : uint8_t
{
    ePreferGPU, // Use GPU when possible, fall back to CPU
    eForceGPU, // Use GPU only, fail if not possible
    eForceCPU // Always use CPU generation
};

// GPU-based mipmap generation
auto generateMipmapsGPU(vk::Device* pDevice, vk::Queue* pQueue, vk::Image* pImage, uint32_t width, uint32_t height,
                        uint32_t mipLevels, Filter filterMode = Filter::Linear,
                        MipmapGenerationMode mode = MipmapGenerationMode::ePreferGPU) -> Expected<bool>;

// Cache utilities
auto encodeToCacheFile(ImageData* pImageData, const std::string& cachePath) -> Expected<bool>;
} // namespace aph
