#ifndef VKLUTILS_H_
#define VKLUTILS_H_

#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>
#include <vulkan/vulkan.h>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000


#define VK_CHECK_RESULT(f)                                                                                \
    {                                                                                                     \
        VkResult res = (f);                                                                               \
        if (res != VK_SUCCESS) {                                                                          \
            std::cout << "Fatal : VkResult is \"" << vkl::utils::errorString(res) << "\" in " << __FILE__ \
                      << " at line " << __LINE__ << "\n";                                                 \
            assert(res == VK_SUCCESS);                                                                    \
        }                                                                                                 \
    }

namespace vkl::utils
{
std::string errorString(VkResult errorCode);
std::vector<char> readFile(const std::string &filename);
}

#endif // VKLUTILS_H_
