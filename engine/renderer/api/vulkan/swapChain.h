#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "common/window.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl
{
class VulkanDevice;
class VulkanImage;
class VulkanQueue;

struct SwapChainCreateInfo
{
    VkSurfaceKHR surface;
    void *windowHandle;
};

class VulkanSwapChain : public ResourceHandle<VkSwapchainKHR>
{
public:
    VulkanSwapChain(const SwapChainCreateInfo &createInfo, VulkanDevice *pDevice);

    VkResult acquireNextImage(uint32_t *pImageIndex, VkSemaphore semaphore,
                              VkFence fence = VK_NULL_HANDLE) const;

    VkResult presentImage(const uint32_t& imageIdx, VulkanQueue *pQueue, const std::vector<VkSemaphore>& waitSemaphores);

public:
    VkFormat getSurfaceFormat() const { return m_surfaceFormat.format; }
    VkExtent2D getExtent() const { return m_extent; }
    uint32_t getImageCount() const { return m_images.size(); }
    VulkanImage *getImage(uint32_t idx) const { return m_images[idx]; }

private:
    VulkanDevice *m_device;
    std::vector<VulkanImage *> m_images;

    VkSurfaceKHR m_surface;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkExtent2D m_extent;

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
}  // namespace vkl

#endif  // SWAPCHAIN_H_
