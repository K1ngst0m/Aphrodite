#include "common.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define BACKWARD_HAS_DW 1
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#include <backward-cpp/backward.hpp>

namespace aph
{
backward::SignalHandling sh;

std::string TracedException::_get_trace()
{
    std::ostringstream ss;

    backward::StackTrace    stackTrace;
    backward::TraceResolver resolver;
    stackTrace.load_here();
    resolver.load_stacktrace(stackTrace);

    ss << "\n\n == backtrace == \n\n";
    for(std::size_t i = 0; i < stackTrace.size(); ++i)
    {
        const backward::ResolvedTrace trace = resolver.resolve(stackTrace[i]);

        ss << "#" << i << " at " << trace.object_function << "\n";
    }
    ss << "\n == backtrace == \n\n";

    return ss.str();
}
}  // namespace aph

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
        for(std::size_t i = 0; i < width * height; ++i)
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
    for(std::size_t idx = 0; idx < 6; idx++)
    {
        skyboxImages[idx] = aph::utils::loadImageFromFile(paths[idx]);
    }
    return skyboxImages;
}
bool readFile(std::string_view filename, std::string& data)
{
    std::ifstream file(filename.data());
    if(!file.is_open())
    {
        CM_LOG_ERR("Failed to open file: %s\n", filename.data());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    data = buffer.str();
    return true;
}
bool readFile(std::string_view filename, std::vector<uint8_t>& data)
{
    std::ifstream file{filename.data(), std::ios::binary | std::ios::ate};
    if(!file)
    {
        CM_LOG_ERR("Failed to open file: %s\n", filename.data());
        return false;
    }

    // Determine the size of the file
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Resize the vector to hold the file data and read the file into the vector
    data.resize(size);

    if(!file.read(reinterpret_cast<char*>(data.data()), size))
    {
        CM_LOG_ERR("Failed to read file: %s\n", filename.data());
        return false;
    }

    return true;
}
}  // namespace aph::utils
