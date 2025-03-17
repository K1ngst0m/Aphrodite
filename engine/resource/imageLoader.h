#pragma once

#include "api/vulkan/device.h"

namespace aph
{
enum class ImageContainerType
{
    Default = 0,
    Ktx,
    Png,
    Jpg,
};

struct ImageInfo
{
    uint32_t width = {};
    uint32_t height = {};
    std::vector<uint8_t> data = {};
};

struct ImageLoadInfo
{
    std::string debugName = {};
    std::variant<std::string, ImageInfo> data;
    ImageContainerType containerType = { ImageContainerType::Default };
    vk::ImageCreateInfo createInfo = {};
};
} // namespace aph

namespace aph::loader::image
{
std::shared_ptr<aph::ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY = false);

std::array<std::shared_ptr<aph::ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths);

bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data);

bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data);
} // namespace aph::loader::image
