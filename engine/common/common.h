#ifndef VKLCOMMON_H_
#define VKLCOMMON_H_

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <stack>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb/stb_image.h>

namespace aph
{
enum class Result
{
    SUCCESS         = 0,
    NOT_READY       = 1,
    TIMEOUT         = 2,
    INCOMPLETE      = 5,
    ERROR_UNKNOWN   = -1,
};

enum class Format
{
    UNDEFINED                  = 0,
    R4G4_UNORM_PACK8           = 1,
    R4G4B4A4_UNORM_PACK16      = 2,
    B4G4R4A4_UNORM_PACK16      = 3,
    R8_UNORM                   = 9,
    R8_SNORM                   = 10,
    R8_USCALED                 = 11,
    R8_SSCALED                 = 12,
    R8_UINT                    = 13,
    R8_SINT                    = 14,
    R8_SRGB                    = 15,
    R8G8_UNORM                 = 16,
    R8G8_SNORM                 = 17,
    R8G8_USCALED               = 18,
    R8G8_SSCALED               = 19,
    R8G8_UINT                  = 20,
    R8G8_SINT                  = 21,
    R8G8_SRGB                  = 22,
    R8G8B8_UNORM               = 23,
    R8G8B8_SNORM               = 24,
    R8G8B8_USCALED             = 25,
    R8G8B8_SSCALED             = 26,
    R8G8B8_UINT                = 27,
    R8G8B8_SINT                = 28,
    R8G8B8_SRGB                = 29,
    B8G8R8_UNORM               = 30,
    B8G8R8_SNORM               = 31,
    B8G8R8_USCALED             = 32,
    B8G8R8_SSCALED             = 33,
    B8G8R8_UINT                = 34,
    B8G8R8_SINT                = 35,
    B8G8R8_SRGB                = 36,
    R8G8B8A8_UNORM             = 37,
    R8G8B8A8_SNORM             = 38,
    R8G8B8A8_USCALED           = 39,
    R8G8B8A8_SSCALED           = 40,
    R8G8B8A8_UINT              = 41,
    R8G8B8A8_SINT              = 42,
    R8G8B8A8_SRGB              = 43,
    B8G8R8A8_UNORM             = 44,
    B8G8R8A8_SNORM             = 45,
    B8G8R8A8_USCALED           = 46,
    B8G8R8A8_SSCALED           = 47,
    B8G8R8A8_UINT              = 48,
    B8G8R8A8_SINT              = 49,
    B8G8R8A8_SRGB              = 50,
    A8B8G8R8_UNORM_PACK32      = 51,
    A8B8G8R8_SNORM_PACK32      = 52,
    A8B8G8R8_USCALED_PACK32    = 53,
    A8B8G8R8_SSCALED_PACK32    = 54,
    A8B8G8R8_UINT_PACK32       = 55,
    A8B8G8R8_SINT_PACK32       = 56,
    A8B8G8R8_SRGB_PACK32       = 57,
    A2R10G10B10_UNORM_PACK32   = 58,
    A2R10G10B10_SNORM_PACK32   = 59,
    A2R10G10B10_USCALED_PACK32 = 60,
    A2R10G10B10_SSCALED_PACK32 = 61,
    A2R10G10B10_UINT_PACK32    = 62,
    A2R10G10B10_SINT_PACK32    = 63,
    A2B10G10R10_UNORM_PACK32   = 64,
    A2B10G10R10_SNORM_PACK32   = 65,
    A2B10G10R10_USCALED_PACK32 = 66,
    A2B10G10R10_SSCALED_PACK32 = 67,
    A2B10G10R10_UINT_PACK32    = 68,
    A2B10G10R10_SINT_PACK32    = 69,
    R16_UNORM                  = 70,
    R16_SNORM                  = 71,
    R16_USCALED                = 72,
    R16_SSCALED                = 73,
    R16_UINT                   = 74,
    R16_SINT                   = 75,
    R16_SFLOAT                 = 76,
    R16G16_UNORM               = 77,
    R16G16_SNORM               = 78,
    R16G16_USCALED             = 79,
    R16G16_SSCALED             = 80,
    R16G16_UINT                = 81,
    R16G16_SINT                = 82,
    R16G16_SFLOAT              = 83,
    R16G16B16_UNORM            = 84,
    R16G16B16_SNORM            = 85,
    R16G16B16_USCALED          = 86,
    R16G16B16_SSCALED          = 87,
    R16G16B16_UINT             = 88,
    R16G16B16_SINT             = 89,
    R16G16B16_SFLOAT           = 90,
    R16G16B16A16_UNORM         = 91,
    R16G16B16A16_SNORM         = 92,
    R16G16B16A16_USCALED       = 93,
    R16G16B16A16_SSCALED       = 94,
    R16G16B16A16_UINT          = 95,
    R16G16B16A16_SINT          = 96,
    R16G16B16A16_SFLOAT        = 97,
    R32_UINT                   = 98,
    R32_SINT                   = 99,
    R32_SFLOAT                 = 100,
    R32G32_UINT                = 101,
    R32G32_SINT                = 102,
    R32G32_SFLOAT              = 103,
    R32G32B32_UINT             = 104,
    R32G32B32_SINT             = 105,
    R32G32B32_SFLOAT           = 106,
    R32G32B32A32_UINT          = 107,
    R32G32B32A32_SINT          = 108,
    R32G32B32A32_SFLOAT        = 109,
    R64_UINT                   = 110,
    R64_SINT                   = 111,
    R64_SFLOAT                 = 112,
    R64G64_UINT                = 113,
    R64G64_SINT                = 114,
    R64G64_SFLOAT              = 115,
    R64G64B64_UINT             = 116,
    R64G64B64_SINT             = 117,
    R64G64B64_SFLOAT           = 118,
    R64G64B64A64_UINT          = 119,
    R64G64B64A64_SINT          = 120,
    R64G64B64A64_SFLOAT        = 121,
    B10G11R11_UFLOAT_PACK32    = 122,
    D16_UNORM                  = 124,
    D32_SFLOAT                 = 126,
    S8_UINT                    = 127,
    FORMAT_MAX_ENUM            = 0x7FFFFFFF
};

enum class BaseType
{
    BOOL   = 0,
    CHAR   = 1,
    INT    = 2,
    UINT   = 3,
    UINT64 = 4,
    HALF   = 5,
    FLOAT  = 6,
    DOUBLE = 7,
    STRUCT = 8,
};

struct ImageInfo
{
    uint32_t             width  = {};
    uint32_t             height = {};
    std::vector<uint8_t> data   = {};
    Format               format = {Format::UNDEFINED};
};
}  // namespace aph

namespace aph::utils
{
constexpr uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
template <typename T>
typename std::underlying_type<T>::type getUnderLyingType(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}
std::shared_ptr<ImageInfo>                loadImageFromFile(std::string_view path, bool isFlipY = false);
std::array<std::shared_ptr<ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths);
}  // namespace aph::utils

#endif  // VKLCOMMON_H_
