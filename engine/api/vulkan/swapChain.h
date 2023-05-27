#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "wsi/wsi.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Image;
class Queue;

struct SwapChainCreateInfo
{
    Instance* instance;
    WSI*      wsi;
};

class SwapChain : public ResourceHandle<VkSwapchainKHR, SwapChainCreateInfo>
{
public:
    SwapChain(const SwapChainCreateInfo& createInfo, Device* pDevice);
    ~SwapChain();

    VkResult acquireNextImage(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);

    VkResult presentImage(Queue* pQueue, const std::vector<VkSemaphore>& waitSemaphores);

    void reCreate();

public:
    VkFormat getFormat() const { return m_surfaceFormat.format; }
    uint32_t getWidth() const { return m_extent.width; }
    uint32_t getHeight() const { return m_extent.height; }

    Image* getImage() const { return m_images[m_imageIdx].get(); }

private:
    Instance*                           m_pInstance{};
    Device*                             m_pDevice{};
    WSI*                                m_pWSI{};
    std::vector<std::unique_ptr<Image>> m_images{};

    VkSurfaceKHR       m_surface{};
    VkSurfaceFormatKHR m_surfaceFormat{};
    VkExtent2D         m_extent{};

    uint32_t m_imageIdx{};

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
}  // namespace aph::vk

#endif  // SWAPCHAIN_H_
