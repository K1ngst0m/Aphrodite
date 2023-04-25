#include "swapChain.h"
#include "device.h"

namespace
{
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;

    VkSurfaceFormatKHR preferedSurfaceFormat;
    VkPresentModeKHR   preferedPresentMode;
    VkExtent2D         preferedExtent;
};

SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device, GLFWwindow* windowHandle)
{
    SwapChainSupportDetails details;

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

    // extent
    {
        if(details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            details.preferedExtent = details.capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(windowHandle, &width, &height);

            VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            actualExtent.width     = std::clamp(actualExtent.width, details.capabilities.minImageExtent.width,
                                                details.capabilities.maxImageExtent.width);
            actualExtent.height    = std::clamp(actualExtent.height, details.capabilities.minImageExtent.height,
                                                details.capabilities.maxImageExtent.height);
            details.preferedExtent = actualExtent;
        }
    }

    return details;
}

}  // namespace

namespace aph
{
VulkanSwapChain::VulkanSwapChain(const SwapChainCreateInfo& createInfo, VulkanDevice* pDevice) :
    m_device(pDevice),
    m_surface(createInfo.surface)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
        m_surface, m_device->getPhysicalDevice()->getHandle(), static_cast<GLFWwindow*>(createInfo.windowHandle));

    uint32_t minImageCount = std::max(swapChainSupport.capabilities.minImageCount + 1, MAX_SWAPCHAIN_IMAGE_COUNT);
    if(swapChainSupport.capabilities.maxImageCount > 0 && minImageCount > swapChainSupport.capabilities.maxImageCount)
    {
        minImageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = m_surface,
        .minImageCount         = minImageCount,
        .imageFormat           = swapChainSupport.preferedSurfaceFormat.format,
        .imageColorSpace       = swapChainSupport.preferedSurfaceFormat.colorSpace,
        .imageExtent           = swapChainSupport.preferedExtent,
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

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device->getHandle(), &swapChainCreateInfo, nullptr, &getHandle()));

    m_surfaceFormat = swapChainSupport.preferedSurfaceFormat;
    m_extent        = swapChainSupport.preferedExtent;

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device->getHandle(), getHandle(), &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_device->getHandle(), getHandle(), &imageCount, images.data());

    // Create an Image class instances to wrap swapchain image handles.
    for(auto handle : images)
    {
        ImageCreateInfo imageCreateInfo = {
            .extent      = { m_extent.width, m_extent.height, 1 },
            .mipLevels   = 1,
            .arrayLayers = 1,
            .usage       = swapChainCreateInfo.imageUsage,
            .samples     = VK_SAMPLE_COUNT_1_BIT,
            .imageType   = VK_IMAGE_TYPE_2D,
            .format      = getFormat(),
            .tiling      = VK_IMAGE_TILING_OPTIMAL,
        };

        m_images.push_back(std::make_unique<VulkanImage>(m_device, imageCreateInfo, handle));
    }
}

VkResult VulkanSwapChain::acquireNextImage(uint32_t* pImageIndex, VkSemaphore semaphore, VkFence fence) const
{
    return vkAcquireNextImageKHR(m_device->getHandle(), getHandle(), UINT64_MAX, semaphore, fence, pImageIndex);
}

VkResult VulkanSwapChain::presentImage(const uint32_t& imageIdx, VulkanQueue* pQueue,
                                       const std::vector<VkSemaphore>& waitSemaphores)
{
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores    = waitSemaphores.data(),
        .swapchainCount     = 1,
        .pSwapchains        = &getHandle(),
        .pImageIndices      = &imageIdx,
        .pResults           = nullptr,  // Optional
    };

    VkResult result = vkQueuePresentKHR(pQueue->getHandle(), &presentInfo);
    return result;
}

VulkanSwapChain::~VulkanSwapChain() = default;
}  // namespace aph
