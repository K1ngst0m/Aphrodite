#ifndef VKLUTILS_H_
#define VKLUTILS_H_

#define VK_NO_PROTOTYPES
#include <volk.h>
#include "vkInit.h"
#include "common/common.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define VK_CHECK_RESULT(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            std::cout << "Fatal : VkResult is \"" << aph::utils::errorString(res) << "\" in " << __FILE__ \
                      << " at line " << __LINE__ << "\n"; \
            assert(res == VK_SUCCESS); \
        } \
    }

namespace aph::utils
{
std::string        errorString(VkResult errorCode);
std::vector<char>  loadSpvFromFile(const std::string& filename);
std::vector<char>  loadGlslFromFile(const std::string& filename);
VkImageAspectFlags getImageAspectFlags(VkFormat format);
VkImageLayout      getDefaultImageLayoutFromUsage(VkImageUsageFlags usage);
}  // namespace aph::utils

#endif  // VKLUTILS_H_
