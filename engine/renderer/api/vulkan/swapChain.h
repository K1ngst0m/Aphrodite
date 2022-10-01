#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "common.h"
#include "device.h"
#include "vkInit.hpp"

namespace vkl {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct SwapChainCreateInfo {
    const void        *pNext;
    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR format;
    VkBool32           tripleBuffer;
};

class VulkanSwapChain {
public:
    void create(const std::shared_ptr<VulkanDevice> &device, VkSurfaceKHR surface, GLFWwindow *window);
    void cleanup();

    VkResult acqureNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const;

    VkFormat         getFormat() const;
    VkExtent2D       getExtent() const;
    uint32_t         getImageCount() const;
    VkPresentInfoKHR getPresentInfo(VkSemaphore *waitSemaphores, const uint32_t *imageIndex);
    VulkanImage     *getImage(uint32_t idx) const;

private:
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

private:
    void                          allocate(GLFWwindow * window);
    std::shared_ptr<VulkanDevice> _device;
    std::vector<VulkanImage *>    _swapChainImages;
    VkSwapchainKHR                _swapChain;

    VkFormat     _swapChainImageFormat;
    VkExtent2D   _swapChainExtent;
    VkSurfaceKHR _surface;

    DeletionQueue _deletionQueue;
};
} // namespace vkl

#endif // SWAPCHAIN_H_
