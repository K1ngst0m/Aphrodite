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
#include <memory>
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
    SUCCESS       = 0,
    NOT_READY     = 1,
    TIMEOUT       = 2,
    INCOMPLETE    = 5,
    ERROR_UNKNOWN = -1,
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
   // amespace aph::utils

#endif  // VKLCOMMON_H_
