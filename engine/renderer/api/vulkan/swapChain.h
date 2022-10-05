#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "device.h"
#include "common/window.h"

namespace vkl {

struct SwapChainCreateInfo {
    const void        *pNext;
    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR format;
    VkBool32           tripleBuffer;
};

class VulkanSwapChain : public ResourceHandle<VkSwapchainKHR> {
public:
    void create(VulkanDevice *device, VkSurfaceKHR surface, WindowData *data);
    void cleanup();

    VkResult acqureNextImage(VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const;

public:
    VkFormat         getFormat() const;
    VkExtent2D       getExtent() const;
    uint32_t         getImageCount() const;
    VulkanImage     *getImage(uint32_t idx) const;

private:
    void allocate(WindowData *windowData);

    VulkanDevice              *_device;
    std::vector<VulkanImage *> _images;

    VkFormat     _imageFormat;
    VkExtent2D   _extent;
    VkSurfaceKHR _surface;

    DeletionQueue _deletionQueue;
};
} // namespace vkl

#endif // SWAPCHAIN_H_
