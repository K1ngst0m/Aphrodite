#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "wsi/wsi.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Image;
class Queue;
class Semaphore;
class Fence;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    SmallVector<VkSurfaceFormatKHR> formats;
    SmallVector<VkPresentModeKHR>   presentModes;

    VkSurfaceFormatKHR preferedSurfaceFormat;
    VkPresentModeKHR   preferedPresentMode;
};

struct SwapChainCreateInfo
{
    Instance* pInstance = {};
    WSI*      pWsi      = {};

    VkFormat     imageFormat;
    VkClearValue clearValue;
    uint32_t     imageCount;
    bool         enableVsync;
    bool         useFlipSwap;
};

class SwapChain : public ResourceHandle<VkSwapchainKHR, SwapChainCreateInfo>
{
public:
    SwapChain(const CreateInfoType& createInfo, Device* pDevice);
    ~SwapChain();

    Result acquireNextImage(VkSemaphore semaphore, Fence* pFence = {});

    Result presentImage(Queue* pQueue, const std::vector<Semaphore*>& waitSemaphores);

    void reCreate();

public:
    uint32_t getWidth() const { return m_extent.width; }
    uint32_t getHeight() const { return m_extent.height; }
    Image*   getImage() const { return m_images[m_imageIdx]; }
    Format   getFormat() const { return utils::getFormatFromVk(m_surfaceFormat.format); }

private:
    Instance*                   m_pInstance{};
    Device*                     m_pDevice{};
    WSI*                        m_pWSI{};
    ThreadSafeObjectPool<Image> m_imagePools;
    SmallVector<Image*>         m_images{};
    SwapChainSupportDetails     swapChainSupport{};

    VkSurfaceKHR       m_surface{};
    VkSurfaceFormatKHR m_surfaceFormat{};
    VkExtent2D         m_extent{};

    uint32_t m_imageIdx{};

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
}  // namespace aph::vk

#endif  // SWAPCHAIN_H_
