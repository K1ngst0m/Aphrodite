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

enum class PresentMode: uint8_t
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

    auto presentImage(ArrayProxy<Semaphore*> waitSemaphores, Image* pImage = {}) -> Result;
    auto reCreate() -> void;

public:
    // Logical dimensions (window space)
    auto getWidth() const -> uint32_t;
    auto getHeight() const -> uint32_t;
    
    // Pixel dimensions (framebuffer space)
    auto getPixelWidth() const -> uint32_t;
    auto getPixelHeight() const -> uint32_t;
    
    // DPI scaling
    auto getDPIScale() const -> float;
    
    auto getFormat() const -> Format;
    auto getImage() -> Image*;

private:
    auto querySwapChainSupport() -> SwapChainSettings;
    auto acquireNextImage(Semaphore* pSemaphore, Fence* pFence = {}) -> Result;

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
