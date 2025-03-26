#include "swapChain.h"
#include "device.h"
#include "vkUtils.h"

namespace
{

} // namespace

namespace aph::vk
{
SwapChain::SwapChain(const CreateInfoType& createInfo, Device* pDevice)
    : ResourceHandle(VK_NULL_HANDLE, createInfo)
    , m_pInstance(createInfo.pInstance)
    , m_pDevice(pDevice)
    , m_pWindowSystem(createInfo.pWindowSystem)
    , m_pQueue(createInfo.pQueue)
{
    APH_ASSERT(createInfo.pInstance);
    APH_ASSERT(createInfo.pWindowSystem);
    APH_ASSERT(createInfo.pQueue);

    // Image count
    m_createInfo = createInfo;
    m_createInfo.imageCount = createInfo.imageCount != 0 ? createInfo.imageCount : 2;

    reCreate();
}

Result SwapChain::acquireNextImage(Semaphore* pSemaphore, Fence* pFence)
{
    APH_ASSERT(pSemaphore);
    auto result = m_pDevice->getHandle().acquireNextImageKHR(
        getHandle(), UINT64_MAX, pSemaphore->getHandle(), pFence ? pFence->getHandle() : VK_NULL_HANDLE, &m_imageIdx);

    if (result == ::vk::Result::eErrorOutOfDateKHR)
    {
        m_imageIdx = -1;
        if (pFence)
        {
            m_pDevice->getHandle().resetFences({ pFence->getHandle() });
        }
        return Result::Success;
    }

    if (result == ::vk::Result::eSuboptimalKHR)
    {
        VK_LOG_INFO(
            "vkAcquireNextImageKHR returned VK_SUBOPTIMAL_KHR. If window was just resized, ignore this message.");
        return Result::Success;
    }

    return utils::getResult(result);
}

Result SwapChain::presentImage(const std::vector<Semaphore*>& waitSemaphores)
{
    std::vector<::vk::Semaphore> vkSemaphores;
    vkSemaphores.reserve(waitSemaphores.size());
    for (auto sem : waitSemaphores)
    {
        vkSemaphores.push_back(sem->getHandle());
    }

    ::vk::Result vkResult = {};
    ::vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(vkSemaphores)
        .setSwapchains({ getHandle() })
        .setImageIndices({ m_imageIdx })
        .setResults(vkResult);
    auto result = m_pQueue->present(presentInfo);
    if (vkResult == ::vk::Result::eSuboptimalKHR)
    {
        VK_LOG_INFO("vkPresentKHR returned VK_SUBOPTIMAL_KHR. If window was just resized, ignore this message.");
        reCreate();
        return Result::Success;
    }
    return result;
}

SwapChain::~SwapChain()
{
    for (auto* image : m_images)
    {
        m_imagePools.free(image);
    }
    m_imagePools.clear();

    m_pInstance->getHandle().destroySurfaceKHR(m_surface, vk_allocator());
};

void SwapChain::reCreate()
{
    APH_VR(m_pDevice->waitIdle());
    for (auto* image : m_images)
    {
        m_imagePools.free(image);
    }
    m_images.clear();
    m_imagePools.clear();

    if (getHandle() != VK_NULL_HANDLE)
    {
        m_pDevice->getHandle().destroySwapchainKHR(getHandle(), vk_allocator());
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        m_pInstance->getHandle().destroySurfaceKHR(m_surface, vk_allocator());
    }

    m_surface = m_createInfo.pWindowSystem->getSurface(m_createInfo.pInstance);
    swapChainSettings = querySwapChainSupport();

    auto& caps = swapChainSettings.capabilities.surfaceCapabilities;
    if ((caps.maxImageCount > 0) && (m_createInfo.imageCount > caps.maxImageCount))
    {
        VK_LOG_WARN("Changed requested SwapChain images {%d} to maximum allowed SwapChain images {%d}",
                    m_createInfo.imageCount, caps.maxImageCount);
        m_createInfo.imageCount = caps.maxImageCount;
    }
    if (m_createInfo.imageCount < caps.minImageCount)
    {
        VK_LOG_WARN("Changed requested SwapChain images {%d} to minimum required SwapChain images {%d}",
                    m_createInfo.imageCount, caps.minImageCount);
        m_createInfo.imageCount = caps.minImageCount;
    }

    uint32_t minImageCount = std::max(caps.minImageCount + 1, MAX_SWAPCHAIN_IMAGE_COUNT);
    if (caps.maxImageCount > 0 && minImageCount > caps.maxImageCount)
    {
        minImageCount = caps.maxImageCount;
    }

    m_extent.width = std::clamp(m_pWindowSystem->getWidth(), caps.minImageExtent.width, caps.maxImageExtent.width);
    m_extent.height = std::clamp(m_pWindowSystem->getHeight(), caps.minImageExtent.height, caps.maxImageExtent.height);

    ::vk::SwapchainCreateInfoKHR swapchain_create_info{};
    {
        SmallVector<uint32_t> queueFamilyIndices{ m_pQueue->getFamilyIndex() };
        swapchain_create_info.setSurface(m_surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(swapChainSettings.surfaceFormat.surfaceFormat.format)
            .setImageColorSpace(swapChainSettings.surfaceFormat.surfaceFormat.colorSpace)
            .setImageExtent(m_extent)
            .setImageArrayLayers(1)
            .setImageUsage(::vk::ImageUsageFlagBits::eTransferDst)
            .setImageSharingMode(::vk::SharingMode::eExclusive)
            .setQueueFamilyIndices(queueFamilyIndices)
            .setPreTransform(swapChainSettings.capabilities.surfaceCapabilities.currentTransform)
            .setCompositeAlpha(::vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setClipped(::vk::True)
            .setPresentMode(swapChainSettings.presentMode);

        auto [result, swapchain_handle] =
            m_pDevice->getHandle().createSwapchainKHR(swapchain_create_info, vk_allocator());
        VK_VR(result);
        m_handle = std::move(swapchain_handle);
    };

    auto [result, images] = m_pDevice->getHandle().getSwapchainImagesKHR(getHandle());
    VK_VR(result);

    // Create an Image class instances to wrap swapchain image handles.
    for (auto handle : images)
    {
        ImageCreateInfo imageCreateInfo = {
            .extent = { m_extent.width, m_extent.height, 1 },
            .mipLevels = 1,
            .arraySize = 1,
            .sampleCount = 1,
            .usage = swapchain_create_info.imageUsage,
            .imageType = ImageType::e2D,
            .format = getFormat(),
        };

        auto pImage = m_imagePools.allocate(m_pDevice, imageCreateInfo, handle);
        APH_VR(m_pDevice->setDebugObjectName(pImage, "swapchain Image"));
        m_images.push_back(pImage);
    }
}

SwapChainSettings SwapChain::querySwapChainSupport()
{
    auto& gpu = m_pDevice->getPhysicalDevice()->getHandle();
    aph::vk::SwapChainSettings details;

    ::vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo{};
    surfaceInfo.setSurface(m_surface);

    // surface cap
    {
        auto [_, capabilities] = gpu.getSurfaceCapabilities2KHR(surfaceInfo);
        details.capabilities = std::move(capabilities);
    }

    // surface format
    {
        auto [_, formats] = gpu.getSurfaceFormats2KHR(surfaceInfo);
        details.surfaceFormat = formats[0];
        auto preferredFormat = m_createInfo.imageFormat == Format::Undefined ?
                                   ::vk::Format::eB8G8R8A8Unorm :
                                   vk::utils::VkCast(m_createInfo.imageFormat);
        for (const auto& availableFormat : formats)
        {
            if (availableFormat.surfaceFormat.format == preferredFormat)
            {
                details.surfaceFormat = availableFormat;
                break;
            }
        }
    }

    // surface present mode
    {
        auto [_, presentModes] = gpu.getSurfacePresentModesKHR(m_surface);
        details.presentMode = presentModes[0];
        auto preferredMode = presentModes[0];
        switch (m_createInfo.presentMode)
        {
        case PresentMode::eImmediate:
            preferredMode = ::vk::PresentModeKHR::eImmediate;
        case PresentMode::eVsync:
            if (m_createInfo.imageCount <= 2)
                preferredMode = ::vk::PresentModeKHR::eFifo;
            else
                preferredMode = ::vk::PresentModeKHR::eMailbox;

        case PresentMode::eAdaptiveVsync:
            preferredMode = ::vk::PresentModeKHR::eFifoRelaxed;

        default:
            preferredMode = ::vk::PresentModeKHR::eFifo;
        }
        for (const auto& availablePresentMode : presentModes)
        {
            if (availablePresentMode == preferredMode)
            {
                details.presentMode = availablePresentMode;
            }
        }
    }

    return details;
}
} // namespace aph::vk
