#include "imageLoader.h"
#include "common/profiler.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace aph::loader::image
{
std::shared_ptr<aph::ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY)
{
    APH_PROFILER_SCOPE();
    auto image = std::make_shared<aph::ImageInfo>();
    stbi_set_flip_vertically_on_load(isFlipY);
    int width, height, channels;
    uint8_t* img = stbi_load(path.data(), &width, &height, &channels, 0);
    if (img == nullptr)
    {
        printf("Error in loading the image\n");
        exit(0);
    }
    // printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    image->width = width;
    image->height = height;

    image->data.resize(width * height * 4);
    if (channels == 3)
    {
        std::vector<uint8_t> rgba(image->data.size());
        for (std::size_t i = 0; i < width * height; ++i)
        {
            memcpy(&rgba[4 * i], &img[3 * i], 3);
        }
        memcpy(image->data.data(), rgba.data(), image->data.size());
    }
    else
    {
        memcpy(image->data.data(), img, image->data.size());
    }
    stbi_image_free(img);

    return image;
}
std::array<std::shared_ptr<aph::ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths)
{
    APH_PROFILER_SCOPE();
    std::array<std::shared_ptr<aph::ImageInfo>, 6> skyboxImages;
    for (std::size_t idx = 0; idx < 6; idx++)
    {
        skyboxImages[idx] = loadImageFromFile(paths[idx]);
    }
    return skyboxImages;
}
bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(false);
    return false;
}
bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    APH_PROFILER_SCOPE();
    auto img = loadImageFromFile(path.c_str());

    if (img == nullptr)
    {
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;

    textureCI.extent = {
        .width = img->width,
        .height = img->height,
        .depth = 1,
    };

    textureCI.format = aph::Format::RGBA8_UNORM;

    data = img->data;

    return true;
}
} // namespace aph::loader::image
