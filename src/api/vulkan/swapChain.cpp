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
    APH_ASSERT(createInfo.pInstance, "Instance cannot be null");
    APH_ASSERT(pDevice, "Device cannot be null");
    APH_ASSERT(createInfo.pWindowSystem, "Window system cannot be null");
    APH_ASSERT(createInfo.pQueue, "Queue cannot be null");
    reCreate();

    m_pWindowSystem->registerEvent(
        [this](const WindowResizeEvent& /*event*/)
        {
            reCreate();
            return true;
        });
}

auto SwapChain::acquireNextImage(Semaphore* pSemaphore, Fence* pFence) -> Result
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(getHandle() != VK_NULL_HANDLE, "SwapChain handle cannot be null");

    ::vk::Result result{};
    {
        APH_PROFILER_SCOPE_NAME("vkAcuiqreNextImageKHR");
        result = m_pDevice->getHandle().acquireNextImageKHR(getHandle(), UINT64_MAX,
                                                            pSemaphore ? pSemaphore->getHandle() : VK_NULL_HANDLE,
                                                            pFence ? pFence->getHandle() : VK_NULL_HANDLE, &m_imageIdx);
    }

    if (result == ::vk::Result::eErrorOutOfDateKHR)
    {
        m_imageIdx = 0U;
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

auto SwapChain::presentImage(ArrayProxy<Semaphore*> waitSemaphores, Image* pImage) -> Result
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(getHandle() != VK_NULL_HANDLE, "SwapChain handle cannot be null");
    APH_ASSERT(m_pQueue, "Presentation queue cannot be null");

    // Validate waitSemaphores
    for (auto* sem : waitSemaphores)
    {
        APH_ASSERT(sem, "Wait semaphore cannot be null");
    }

    SmallVector<::vk::Semaphore> vkSemaphores;
    vkSemaphores.reserve(waitSemaphores.size());
    for (auto* sem : waitSemaphores)
    {
        vkSemaphores.push_back(sem->getHandle());
    }

    if (pImage)
    {
        APH_ASSERT(m_pAcquireImageFence, "Acquire image fence cannot be null");
        APH_VERIFY_RESULT(acquireNextImage({}, m_pAcquireImageFence));

        APH_ASSERT(m_imageIdx >= 0 && m_imageIdx < m_imageResources.size(), "Invalid swapchain image index");
        m_pAcquireImageFence->wait();
        m_pAcquireImageFence->reset();

        const auto& imageRes = m_imageResources[m_imageIdx];
        APH_ASSERT(imageRes.pPresentSemaphore, "Present semaphore cannot be null");
        APH_ASSERT(imageRes.pImage, "Swapchain image cannot be null");

        vkSemaphores.push_back(imageRes.pPresentSemaphore->getHandle());

        m_pDevice->executeCommand(m_pDevice->getQueue(aph::QueueType::Transfer),
                                  [this, pImage](auto* pCopyCmd)
                                  {
                                      APH_ASSERT(pCopyCmd, "Command buffer cannot be null");
                                      auto pSwapchainImage = getImage();
                                      auto pOutImage       = pImage;

                                      APH_ASSERT(pSwapchainImage, "Swapchain image cannot be null");
                                      APH_ASSERT(pOutImage, "Source image cannot be null");

                                      pCopyCmd->insertBarrier({
                                          {
                                           .pImage       = pOutImage,
                                           .currentState = ResourceState::RenderTarget,
                                           .newState     = ResourceState::CopySource,
                                           },
                                          {
                                           .pImage       = pSwapchainImage,
                                           .currentState = ResourceState::Undefined,
                                           .newState     = ResourceState::CopyDest,
                                           },
                                      });

                                      if (pOutImage->getWidth() == pSwapchainImage->getWidth() &&
                                          pOutImage->getHeight() == pSwapchainImage->getHeight() &&
                                          pOutImage->getDepth() == pSwapchainImage->getDepth())
                                      {
                                          VK_LOG_DEBUG("copy image to swapchain.");
                                          pCopyCmd->copy(pOutImage, pSwapchainImage);
                                      }
                                      else
                                      {
                                          VK_LOG_DEBUG("blit image to swapchain.");
                                          pCopyCmd->blit(pOutImage, pSwapchainImage);
                                      }

                                      pCopyCmd->insertBarrier({
                                          {
                                           .pImage       = pOutImage,
                                           .currentState = ResourceState::Undefined,
                                           .newState     = ResourceState::RenderTarget,
                                           },
                                          {
                                           .pImage       = pSwapchainImage,
                                           .currentState = ResourceState::CopyDest,
                                           .newState     = ResourceState::Present,
                                           },
                                      });
                                  },
                                  {}, { imageRes.pPresentSemaphore });
    }

    ::vk::Result vkResult = {};
    ::vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(vkSemaphores)
        .setSwapchains({ getHandle() })
        .setImageIndices({ m_imageIdx })
        .setResults(vkResult);

    APH_ASSERT(m_imageIdx >= 0 && m_imageIdx < m_imageResources.size(),
               "Invalid swapchain image index for presentation");
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
    for (const auto& imageResource : m_imageResources)
    {
        m_imagePools.free(imageResource.pImage);
        APH_VERIFY_RESULT(m_pDevice->releaseSemaphore(imageResource.pPresentSemaphore));
    }
    m_imagePools.clear();
    APH_VERIFY_RESULT(m_pDevice->releaseFence(m_pAcquireImageFence));

    if (m_surface != VK_NULL_HANDLE)
    {
        m_pInstance->getHandle().destroySurfaceKHR(m_surface, vk_allocator());
    }
};

void SwapChain::reCreate()
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(m_pDevice, "Device cannot be null");
    APH_ASSERT(m_pWindowSystem, "Window system cannot be null");
    APH_ASSERT(m_pInstance, "Instance cannot be null");

    APH_VERIFY_RESULT(m_pDevice->waitIdle());

    // Setup variables needed for swapchain recreation
    ::vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    ::vk::SwapchainKHR swapchainHandle{};
    std::vector<::vk::Image> swapchainImages;
    ImageCreateInfo imageCreateInfo{};

    //
    // 1. Cleanup existing resources
    //
    {
        for (const auto& imageResource : m_imageResources)
        {
            m_imagePools.free(imageResource.pImage);
            APH_VERIFY_RESULT(m_pDevice->releaseSemaphore(imageResource.pPresentSemaphore));
        }
        m_imageResources.clear();
        m_imagePools.clear();

        if (getHandle() != VK_NULL_HANDLE)
        {
            m_pDevice->getHandle().destroySwapchainKHR(getHandle(), vk_allocator());
        }

        if (m_surface != VK_NULL_HANDLE)
        {
            m_pInstance->getHandle().destroySurfaceKHR(m_surface, vk_allocator());
        }
    }

    //
    // 2. Create new surface and query capabilities
    //
    {
        m_surface = m_createInfo.pWindowSystem->getSurface(m_createInfo.pInstance);
        APH_ASSERT(m_surface != VK_NULL_HANDLE, "Failed to create Vulkan surface");

        swapChainSettings = querySwapChainSupport();
        APH_ASSERT(swapChainSettings.surfaceFormat.surfaceFormat.format != ::vk::Format::eUndefined,
                   "No suitable surface format found");
    }

    //
    // 3. Configure swapchain parameters
    //
    {
        auto& caps = swapChainSettings.capabilities.surfaceCapabilities;

        // Adjust image count based on device limits
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

        // Determine final image count
        uint32_t minImageCount = std::max(caps.minImageCount + 1, MAX_SWAPCHAIN_IMAGE_COUNT);
        if (caps.maxImageCount > 0 && minImageCount > caps.maxImageCount)
        {
            minImageCount = caps.maxImageCount;
        }

        APH_ASSERT(minImageCount > 0, "Swapchain image count must be greater than 0");

        // For high DPI support, get pixel dimensions directly from window system
        // rather than using logical dimensions
        uint32_t pixelWidth  = m_pWindowSystem->getPixelWidth();
        uint32_t pixelHeight = m_pWindowSystem->getPixelHeight();

        // Log information about DPI scaling
        float dpiScale = m_pWindowSystem->getDPIScale();
        if (dpiScale > 1.0f)
        {
            VK_LOG_INFO("Creating SwapChain with DPI scale %.2f: Logical: %dx%d, Framebuffer: %dx%d", dpiScale,
                        m_pWindowSystem->getWidth(), m_pWindowSystem->getHeight(), pixelWidth, pixelHeight);
        }

        // Configure extent based on window pixel dimensions and device limits
        APH_ASSERT(pixelWidth > 0 && pixelHeight > 0, "Window pixel dimensions must be greater than 0");
        APH_ASSERT(pixelWidth <= caps.maxImageExtent.width && pixelHeight <= caps.maxImageExtent.height,
                   "Window dimensions exceed maximum allowed by device");

        m_extent.width  = std::clamp(pixelWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
        m_extent.height = std::clamp(pixelHeight, caps.minImageExtent.height, caps.maxImageExtent.height);

        // Setup swapchain creation info
        SmallVector<uint32_t> queueFamilyIndices{ m_pQueue->getFamilyIndex() };
        swapchainCreateInfo.setSurface(m_surface)
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

        APH_ASSERT(m_extent.width > 0 && m_extent.height > 0, "Swapchain extent cannot be zero");
    }

    //
    // 4. Create the swapchain
    //
    {
        auto [result, handle] = m_pDevice->getHandle().createSwapchainKHR(swapchainCreateInfo, vk_allocator());
        VK_VR(result);
        m_handle = std::move(handle);
        APH_ASSERT(m_handle != VK_NULL_HANDLE, "Failed to create swapchain");

        auto [imageResult, images] = m_pDevice->getHandle().getSwapchainImagesKHR(getHandle());
        VK_VR(imageResult);
        swapchainImages = std::move(images);
        APH_ASSERT(!swapchainImages.empty(), "No swapchain images returned from Vulkan");
    }

    //
    // 5. Create image wrappers for swapchain images
    //
    {
        // Prepare common image creation info
        imageCreateInfo = {
            .extent      = { .width = m_extent.width, .height = m_extent.height, .depth = 1 },
            .mipLevels   = 1,
            .arraySize   = 1,
            .sampleCount = 1,
            .usage       = utils::getImageUsage(swapchainCreateInfo.imageUsage),
            .imageType   = ImageType::e2D,
            .format      = getFormat(),
        };

        // Create an Image class instance for each swapchain image
        for (auto handle : swapchainImages)
        {
            APH_ASSERT(handle != VK_NULL_HANDLE, "SwapChain image handle cannot be null");

            ImageResource imageRes{};

            imageRes.pImage = m_imagePools.allocate(m_pDevice, imageCreateInfo, handle);
            APH_ASSERT(imageRes.pImage, "Failed to allocate image wrapper for swapchain image");
            APH_VERIFY_RESULT(m_pDevice->setDebugObjectName(imageRes.pImage, "swapchain Image"));

            imageRes.pPresentSemaphore = m_pDevice->acquireSemaphore();
            APH_ASSERT(imageRes.pPresentSemaphore, "Failed to acquire present semaphore for swapchain image");

            m_imageResources.push_back(imageRes);
        }

        APH_ASSERT(m_imageResources.size() == swapchainImages.size(),
                   "Mismatch between swapchain images and image resources count");
    }

    //
    // 6. Create auxiliary resources
    //
    {
        if (!m_pAcquireImageFence)
        {
            m_pAcquireImageFence = m_pDevice->acquireFence(false);
            APH_ASSERT(m_pAcquireImageFence, "Failed to create acquire image fence");
        }
    }
}

auto SwapChain::querySwapChainSupport() -> SwapChainSettings
{
    APH_ASSERT(m_pDevice && m_pDevice->getPhysicalDevice(), "Device or physical device is null");
    APH_ASSERT(m_surface != VK_NULL_HANDLE, "Surface cannot be null when querying swapchain support");

    auto& gpu = m_pDevice->getPhysicalDevice()->getHandle();
    aph::vk::SwapChainSettings details;

    ::vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo{};
    surfaceInfo.setSurface(m_surface);

    // surface cap
    {
        auto [result, capabilities] = gpu.getSurfaceCapabilities2KHR(surfaceInfo);
        APH_ASSERT(result == ::vk::Result::eSuccess, "Failed to get surface capabilities");
        details.capabilities = capabilities;
    }

    // surface format
    {
        auto [result, formats] = gpu.getSurfaceFormats2KHR(surfaceInfo);
        APH_ASSERT(result == ::vk::Result::eSuccess, "Failed to get surface formats");
        APH_ASSERT(!formats.empty(), "No surface formats available");

        details.surfaceFormat = formats[0];
        auto preferredFormat  = m_createInfo.imageFormat == Format::Undefined ?
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
        auto [result, presentModes] = gpu.getSurfacePresentModesKHR(m_surface);
        APH_ASSERT(result == ::vk::Result::eSuccess, "Failed to get surface present modes");
        APH_ASSERT(!presentModes.empty(), "No present modes available");

        details.presentMode = presentModes[0];
        auto preferredMode  = presentModes[0];
        switch (m_createInfo.presentMode)
        {
        case PresentMode::eImmediate:
            preferredMode = ::vk::PresentModeKHR::eImmediate;
            break;
        case PresentMode::eVsync:
            if (m_createInfo.imageCount <= 2)
            {
                preferredMode = ::vk::PresentModeKHR::eFifo;
            }
            else
            {
                preferredMode = ::vk::PresentModeKHR::eMailbox;
            }
            break;
        case PresentMode::eAdaptiveVsync:
            preferredMode = ::vk::PresentModeKHR::eFifoRelaxed;
            break;
        default:
            preferredMode = ::vk::PresentModeKHR::eFifo;
            break;
        }
        for (const auto& availablePresentMode : presentModes)
        {
            if (availablePresentMode == preferredMode)
            {
                details.presentMode = availablePresentMode;
                break;
            }
        }
    }

    return details;
}

auto SwapChain::getWidth() const -> uint32_t
{
    return m_pWindowSystem->getWidth();
}

auto SwapChain::getHeight() const -> uint32_t
{
    return m_pWindowSystem->getHeight();
}

auto SwapChain::getPixelWidth() const -> uint32_t
{
    return m_extent.width;
}

auto SwapChain::getPixelHeight() const -> uint32_t
{
    return m_extent.height;
}

auto SwapChain::getDPIScale() const -> float
{
    return m_pWindowSystem->getDPIScale();
}

auto SwapChain::getFormat() const -> Format
{
    return utils::getFormatFromVk(static_cast<VkFormat>(swapChainSettings.surfaceFormat.surfaceFormat.format));
}

auto SwapChain::getImage() -> Image*
{
    return m_imageResources[m_imageIdx].pImage;
}
} // namespace aph::vk
