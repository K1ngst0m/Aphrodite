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

#ifdef APH_DEBUG
#    define VK_CHECK_RESULT(f) \
        { \
            VkResult res = (f); \
            if(res != VK_SUCCESS) \
            { \
                CM_LOG_ERR("Fatal : VkResult is \"%s\" in %s at line %s", aph::vk::utils::errorString(res), __FILE__, \
                           __LINE__); \
                std::abort(); \
            } \
        }
#else
#    define VK_CHECK_RESULT(f) (f)
#endif

namespace aph::vk::utils
{
std::string           errorString(VkResult errorCode);
std::vector<uint32_t> loadSpvFromFile(const std::string& filename);
std::vector<uint32_t> loadGlslFromFile(const std::string& filename);
VkImageAspectFlags    getImageAspect(VkFormat format);
ShaderStage           getStageFromPath(std::string_view path);
VkSampleCountFlagBits getSampleCountFlags(uint32_t numSamples);

}  // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
VkShaderStageFlagBits VkCast(ShaderStage stage);
VkDescriptorType      VkCast(ResourceType type);
VkShaderStageFlags    VkCast(const std::vector<ShaderStage>& stages);
}  // namespace aph::vk::utils

namespace aph
{
constexpr unsigned VULKAN_NUM_DESCRIPTOR_SETS           = 4;
constexpr unsigned VULKAN_NUM_BINDINGS                  = 32;
constexpr unsigned VULKAN_NUM_BINDINGS_BINDLESS_VARYING = 16 * 1024;
constexpr unsigned VULKAN_NUM_ATTACHMENTS               = 8;
constexpr unsigned VULKAN_NUM_VERTEX_ATTRIBS            = 16;
constexpr unsigned VULKAN_NUM_VERTEX_BUFFERS            = 4;
constexpr unsigned VULKAN_PUSH_CONSTANT_SIZE            = 128;
constexpr unsigned VULKAN_MAX_UBO_SIZE                  = 16 * 1024;
constexpr unsigned VULKAN_NUM_USER_SPEC_CONSTANTS       = 8;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS   = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL    = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;
}  // namespace aph

#endif  // VKLUTILS_H_
