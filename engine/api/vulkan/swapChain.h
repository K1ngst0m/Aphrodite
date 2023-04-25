#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "common/window.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanDevice;
class VulkanImage;
class VulkanQueue;

struct SwapChainCreateInfo
{
    VkSurfaceKHR surface;
    void*        windowHandle;
};

class VulkanSwapChain : public ResourceHandle<VkSwapchainKHR, SwapChainCreateInfo>
{
public:
    VulkanSwapChain(const SwapChainCreateInfo& createInfo, VulkanDevice* pDevice);
    ~VulkanSwapChain();

    VkResult acquireNextImage(uint32_t* pImageIndex, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE) const;

    VkResult presentImage(const uint32_t& imageIdx, VulkanQueue* pQueue,
                          const std::vector<VkSemaphore>& waitSemaphores);

public:
    VkFormat   getFormat() const { return m_surfaceFormat.format; }
    uint32_t   getWidth() const { return m_extent.width; }
    uint32_t   getHeight() const { return m_extent.height; }

    VulkanImage* getImage(uint32_t idx) const { return m_images[idx].get(); }

private:
    VulkanDevice*                             m_device{};
    std::vector<std::unique_ptr<VulkanImage>> m_images{};

    VkSurfaceKHR       m_surface{};
    VkSurfaceFormatKHR m_surfaceFormat{};
    VkExtent2D         m_extent{};

    uint32_t m_imageIdx{};

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
}  // namespace aph

#endif  // SWAPCHAIN_H_
