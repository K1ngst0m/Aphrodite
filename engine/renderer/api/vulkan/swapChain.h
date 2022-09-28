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

class VulkanSwapChain {
public:
    void create(const std::shared_ptr<VulkanDevice> &device, VkSurfaceKHR surface, GLFWwindow *window);
    void cleanup();

    VkResult acqureNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const;

    VkFormat         getFormat() const;
    VkExtent2D       getExtent() const;
    uint32_t         getImageCount() const;
    VkImageView      getImageViewWithIdx(uint32_t idx);
    VkPresentInfoKHR getPresentInfo(VkSemaphore *waitSemaphores, const uint32_t *imageIndex);

private:
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

private:
    std::shared_ptr<VulkanDevice> m_device;
    std::vector<VkImage>          m_swapChainImages;
    std::vector<VkImageView>      m_swapChainImageViews;
    VkSwapchainKHR                m_swapChain;

    VkFormat     m_swapChainImageFormat;
    VkExtent2D   m_swapChainExtent;
    VkSurfaceKHR m_surface;

    DeletionQueue m_deletionQueue;
};
} // namespace vkl

#endif // SWAPCHAIN_H_
