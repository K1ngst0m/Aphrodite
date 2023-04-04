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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

namespace aph {
constexpr uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
} // namespace aph

#endif // VKLCOMMON_H_
