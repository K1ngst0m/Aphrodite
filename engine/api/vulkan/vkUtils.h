#ifndef VKLUTILS_H_
#define VKLUTILS_H_

#include "api/gpuResource.h"
#define VK_NO_PROTOTYPES
#include <volk.h>
#include "vkInit.h"
#include "common/common.h"
#include "common/logger.h"

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
            aph::Logger::Get()->error("Fatal : VkResult is \"%s\" in %s at line %s\n", aph::vk::utils::errorString(res), __FILE__, __LINE__); \
            assert(res == VK_SUCCESS); \
        } \
    }

namespace aph::vk::utils
{
std::string        errorString(VkResult errorCode);
std::vector<char>  loadSpvFromFile(const std::string& filename);
std::vector<char>  loadGlslFromFile(const std::string& filename);
VkImageAspectFlags getImageAspect(VkFormat format);
}  // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
VkShaderStageFlagBits VkCast(ShaderStage stage);
VkDescriptorType      VkCast(ResourceType type);
VkShaderStageFlags    VkCast(const std::vector<ShaderStage>& stages);
}  // namespace aph::vk::utils

#endif  // VKLUTILS_H_
