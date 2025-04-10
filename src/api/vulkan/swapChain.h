#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/arrayProxy.h"
#include "forward.h"
#include "vkUtils.h"
#include "wsi/wsi.h"

namespace aph::vk
{
struct SwapChainSettings
{
    ::vk::SurfaceCapabilities2KHR capabilities;
    ::vk::SurfaceFormat2KHR surfaceFormat;
    ::vk::PresentModeKHR presentMode;
};

enum class PresentMode
{
    eImmediate,
    eVsync,
    eAdaptiveVsync,
};

struct SwapChainCreateInfo
{
    Instance* pInstance         = {};
    WindowSystem* pWindowSystem = {};
    Queue* pQueue               = {};

    Format imageFormat = Format::Undefined;
    uint32_t imageCount;
    PresentMode presentMode = PresentMode::eVsync;
};

class SwapChain : public ResourceHandle<::vk::SwapchainKHR, SwapChainCreateInfo>
{
public:
    SwapChain(const CreateInfoType& createInfo, Device* pDevice);
    ~SwapChain();

    Result presentImage(ArrayProxy<Semaphore*> waitSemaphores, Image* pImage = {});

    void reCreate();

public:
    uint32_t getWidth() const
    {
        return m_extent.width;
    }
    uint32_t getHeight() const
    {
        return m_extent.height;
    }
    Format getFormat() const
    {
        return utils::getFormatFromVk(static_cast<VkFormat>(swapChainSettings.surfaceFormat.surfaceFormat.format));
    }
    Image* getImage()
    {
        return m_imageResources[m_imageIdx].pImage;
    }

private:
    SwapChainSettings querySwapChainSupport();
    Result acquireNextImage(Semaphore* pSemaphore, Fence* pFence = {});

private:
    Instance* m_pInstance{};
    Device* m_pDevice{};
    WindowSystem* m_pWindowSystem{};
    Queue* m_pQueue{};
    ThreadSafeObjectPool<Image> m_imagePools;

    Fence* m_pAcquireImageFence = {};

    struct ImageResource
    {
        Image* pImage                = {};
        Semaphore* pPresentSemaphore = {};
    };

    SmallVector<ImageResource> m_imageResources;

    SwapChainSettings swapChainSettings{};

    ::vk::SurfaceKHR m_surface{};
    ::vk::Extent2D m_extent{};

    uint32_t m_imageIdx{};

    constexpr static uint32_t MAX_SWAPCHAIN_IMAGE_COUNT = 3;
};
} // namespace aph::vk
