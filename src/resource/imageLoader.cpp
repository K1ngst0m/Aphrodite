#include "imageLoader.h"

#include "common/profiler.h"
#include "common/common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace aph
{
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

ImageFormat getFormatFromChannels(int channels)
{
    switch (channels)
    {
    case 1: return ImageFormat::R8_UNORM;
    case 2: return ImageFormat::R8G8_UNORM;
    case 3: return ImageFormat::R8G8B8_UNORM;
    case 4: return ImageFormat::R8G8B8A8_UNORM;
    default: return ImageFormat::Unknown;
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
            baseMip.data[4 * i + 3] = 255;            // A (full opacity)
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
