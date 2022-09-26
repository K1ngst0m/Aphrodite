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
    void create(VulkanDevice *device, VkSurfaceKHR surface, GLFWwindow *window);
    void cleanup();
    void recreate();

    void createDepthResources(VkQueue transferQueue);
    void createFramebuffers(VkRenderPass renderPass);

    VkResult              acqureNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const;
    VkPresentInfoKHR      getPresentInfo(VkSemaphore *waitSemaphores, const uint32_t *imageIndex);
    VkRenderPassBeginInfo getRenderPassBeginInfo(VkRenderPass renderPass, const std::vector<VkClearValue> &clearValues, uint32_t imageIdx);

    VkFormat   getFormat() const;
    VkExtent2D getExtent() const;
    uint32_t   getImageCount() const;

private:
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

private:
    VulkanDevice              *m_device = nullptr;
    DeletionQueue              m_deletionQueue;
    std::vector<VkImage>       m_swapChainImages;
    std::vector<VkImageView>   m_swapChainImageViews;
    vkl::VulkanTexture         m_depthAttachment;
    std::vector<VkFramebuffer> m_framebuffers;
    VkSwapchainKHR             m_swapChain;
    VkFormat                   m_swapChainImageFormat;
    VkExtent2D                 m_swapChainExtent;
    VkSurfaceKHR               m_surface;
};
} // namespace vkl

#endif // SWAPCHAIN_H_
