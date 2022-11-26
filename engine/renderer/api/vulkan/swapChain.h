#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "common/window.h"
#include "device.h"

namespace vkl {

struct SwapChainCreateInfo {
    const void        *pNext;
    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR format;
    VkBool32           tripleBuffer;
};

class VulkanSwapChain : public ResourceHandle<VkSwapchainKHR> {
public:
    static VulkanSwapChain *Create(VulkanDevice *device, VkSurfaceKHR surface, WindowData *data);
    void                    allocateImages(WindowData *data);
    void                    cleanupImages();

    VkResult acquireNextImage(uint32_t *pImageIndex, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE) const;

public:
    VkFormat     getImageFormat() const;
    VkExtent2D   getExtent() const;
    uint32_t     getImageCount() const;
    VulkanImage *getImage(uint32_t idx) const;

private:
    VulkanDevice              *_device;
    std::vector<VulkanImage *> _images;

    uint32_t        _imageCount;
    VkColorSpaceKHR _imageColorSpace;
    VkFormat        _imageFormat;
    VkExtent2D      _extent;
    VkSurfaceKHR    _surface;

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
} // namespace vkl

#endif // SWAPCHAIN_H_
