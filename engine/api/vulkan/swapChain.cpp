#include "swapChain.h"
#include "device.h"

namespace
{
aph::vk::SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device, aph::WSI* wsi)
{
    aph::vk::SwapChainSupportDetails details;

    // surface cap
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    }

    // surface format
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if(formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        details.preferedSurfaceFormat = details.formats[0];
        for(const auto& availableFormat : details.formats)
        {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                details.preferedSurfaceFormat = availableFormat;
                break;
            }
        }
    }

    // surface present mode
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if(presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        details.preferedPresentMode = details.presentModes[0];
        for(const auto& availablePresentMode : details.presentModes)
        {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                details.preferedPresentMode = availablePresentMode;
            }
        }
    }

    return details;
}

}  // namespace

namespace aph::vk
{
SwapChain::SwapChain(const CreateInfoType& createInfo, Device* pDevice) :
    ResourceHandle(VK_NULL_HANDLE, createInfo),
    m_pInstance(createInfo.pInstance),
    m_pDevice(pDevice),
    m_pWSI(createInfo.pWsi)
{
    APH_ASSERT(createInfo.pInstance);
    APH_ASSERT(createInfo.pWsi);

    // Image count
    m_createInfo            = createInfo;
    m_createInfo.imageCount = createInfo.imageCount != 0 ? createInfo.imageCount : 2;

    reCreate();
}

Result SwapChain::acquireNextImage(VkSemaphore semaphore, Fence* pFence)
{
    VkResult res = VK_SUCCESS;
    res          = vkAcquireNextImageKHR(m_pDevice->getHandle(), getHandle(), UINT64_MAX, semaphore,
                                pFence ? pFence->getHandle() : VK_NULL_HANDLE, &m_imageIdx);

    if(res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_imageIdx = -1;
        if(pFence)
        {
            vkResetFences(m_pDevice->getHandle(), 1, &pFence->getHandle());
        }
        return Result::Success;
    }

    if(res == VK_SUBOPTIMAL_KHR)
    {
        VK_LOG_INFO(
            "vkAcquireNextImageKHR returned VK_SUBOPTIMAL_KHR. If window was just resized, ignore this message.");
        return Result::Success;
    }

    return utils::getResult(res);
}

Result SwapChain::presentImage(Queue* pQueue, const std::vector<Semaphore*>& waitSemaphores)
{
    m_pDevice->executeSingleCommands(pQueue, [&](CommandBuffer* cmd) {
        aph::vk::ImageBarrier barrier{
            .pImage       = m_images[m_imageIdx],
            .currentState = m_images[m_imageIdx]->getResourceState(),
            .newState     = aph::RESOURCE_STATE_PRESENT,
        };
        cmd->insertBarrier({barrier});
    });

    std::vector<VkSemaphore> vkSemaphores;
    vkSemaphores.reserve(waitSemaphores.size());
    for(auto sem : waitSemaphores)
    {
        vkSemaphores.push_back(sem->getHandle());
    }

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores    = vkSemaphores.data(),
        .swapchainCount     = 1,
        .pSwapchains        = &getHandle(),
        .pImageIndices      = &m_imageIdx,
        .pResults           = nullptr,  // Optional
    };

    VkResult result = vkQueuePresentKHR(pQueue->getHandle(), &presentInfo);
    return utils::getResult(result);
}

SwapChain::~SwapChain()
{
    // TODO figure out why pool clear could not called object deleter
    for(auto* image : m_images)
    {
        m_imagePools.free(image);
    }
    m_imagePools.clear();

    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, nullptr);
};

void SwapChain::reCreate()
{
    m_pDevice->waitIdle();
    for(auto* image : m_images)
    {
        m_imagePools.free(image);
    }
    m_images.clear();
    m_imagePools.clear();
    if(getHandle() != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_pDevice->getHandle(), getHandle(), vkAllocator());
    }

    if(m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, nullptr);
    }

    m_surface        = m_createInfo.pWsi->getSurface(m_createInfo.pInstance);
    swapChainSupport = querySwapChainSupport(m_surface, m_pDevice->getPhysicalDevice()->getHandle(), m_pWSI);

    auto& caps = swapChainSupport.capabilities;
    if((caps.maxImageCount > 0) && (m_createInfo.imageCount > caps.maxImageCount))
    {
        VK_LOG_WARN("Changed requested SwapChain images {%d} to maximum allowed SwapChain images {%d}",
                    m_createInfo.imageCount, caps.maxImageCount);
        m_createInfo.imageCount = caps.maxImageCount;
    }
    if(m_createInfo.imageCount < caps.minImageCount)
    {
        VK_LOG_WARN("Changed requested SwapChain images {%d} to minimum required SwapChain images {%d}",
                    m_createInfo.imageCount, caps.minImageCount);
        m_createInfo.imageCount = caps.minImageCount;
    }

    uint32_t minImageCount = std::max(caps.minImageCount + 1, MAX_SWAPCHAIN_IMAGE_COUNT);
    if(caps.maxImageCount > 0 && minImageCount > caps.maxImageCount)
    {
        minImageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkExtent2D extent = {};

    extent.width  = std::clamp(m_pWSI->getWidth(), caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(m_pWSI->getHeight(), caps.minImageExtent.height, caps.maxImageExtent.height);

    VkSwapchainCreateInfoKHR swapChainCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = m_surface,
        .minImageCount         = minImageCount,
        .imageFormat           = swapChainSupport.preferedSurfaceFormat.format,
        .imageColorSpace       = swapChainSupport.preferedSurfaceFormat.colorSpace,
        .imageExtent           = extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = swapChainSupport.preferedPresentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
    };

    _VR(vkCreateSwapchainKHR(m_pDevice->getHandle(), &swapChainCreateInfo, vk::vkAllocator(), &getHandle()));

    m_surfaceFormat = swapChainSupport.preferedSurfaceFormat;
    m_extent        = extent;

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_pDevice->getHandle(), getHandle(), &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_pDevice->getHandle(), getHandle(), &imageCount, images.data());

    // Create an Image class instances to wrap swapchain image handles.
    for(auto handle : images)
    {
        ImageCreateInfo imageCreateInfo = {
            .extent    = {m_extent.width, m_extent.height, 1},
            .mipLevels = 1,
            .arraySize = 1,
            .usage     = swapChainCreateInfo.imageUsage,
            .samples   = VK_SAMPLE_COUNT_1_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = getFormat(),
        };

        m_images.push_back(m_imagePools.allocate(m_pDevice, imageCreateInfo, handle));
    }
}
}  // namespace aph::vk
