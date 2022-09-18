#ifndef VKLCOMMON_H_
#define VKLCOMMON_H_

#include <list>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spirv_reflect/spirv_reflect.h>

#include <stb_image.h>

namespace vkl {
    using ColorValue = glm::vec4;
}

#endif // VKLCOMMON_H_
