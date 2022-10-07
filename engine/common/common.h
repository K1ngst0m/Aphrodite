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

namespace vkl {
struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }

        deletors.clear();
    }
};

inline uint32_t calculateFullMipLevels(uint32_t width, uint32_t height, uint32_t depth) {
    return static_cast<uint32_t>(log2f(static_cast<float>(std::max(std::max(width, height), depth)))) + 1;
}
} // namespace vkl

#endif // VKLCOMMON_H_
