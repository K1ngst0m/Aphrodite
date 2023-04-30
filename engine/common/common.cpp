#define STB_IMAGE_IMPLEMENTATION
#include "common.h"

namespace aph::utils
{
std::shared_ptr<ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY)
{
    auto image = std::make_shared<ImageInfo>();
    stbi_set_flip_vertically_on_load(isFlipY);
    int      width, height, channels;
    uint8_t* img = stbi_load(path.data(), &width, &height, &channels, 0);
    if(img == nullptr)
    {
        printf("Error in loading the image\n");
        exit(0);
    }
    // printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    image->width  = width;
    image->height = height;

    image->data.resize(width * height * 4);
    if(channels == 3)
    {
        std::vector<uint8_t> rgba(image->data.size());
        for(size_t i = 0; i < width * height; ++i)
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
std::array<std::shared_ptr<ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths)
{
    std::array<std::shared_ptr<ImageInfo>, 6> skyboxImages;
    for(uint32_t idx = 0; idx < 6; idx++)
    {
        skyboxImages[idx] = aph::utils::loadImageFromFile(paths[idx]);
    }
    return skyboxImages;
}
}  // namespace aph::utils
