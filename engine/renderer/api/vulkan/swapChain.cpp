#include "swapChain.h"

namespace vkl {
SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void VulkanSwapChain::create(const std::shared_ptr<VulkanDevice>& device, VkSurfaceKHR surface, GLFWwindow* window) {
    m_device = device;
    m_surface = surface;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_device->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = vkl::utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR   presentMode   = vkl::utils::chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D         extent        = vkl::utils::chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,

        .surface = m_surface,

        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

        .preTransform   = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode    = presentMode,
        .clipped        = VK_TRUE,
        .oldSwapchain   = VK_NULL_HANDLE,
    };

    std::array<uint32_t, 2> queueFamilyIndices = {m_device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS),
                                                  m_device->GetQueueFamilyIndices(DeviceQueueType::PRESENT)};

    if (m_device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS) != m_device->GetQueueFamilyIndices(DeviceQueueType::PRESENT)) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapChain));

    vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent      = extent;

    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        m_swapChainImageViews[i] = m_device->createImageView(m_swapChainImages[i], m_swapChainImageFormat);
    }

    m_deletionQueue.push_function([&]() {
        cleanup();
    });
}
void VulkanSwapChain::cleanup() {
    for (auto &m_swapChainImageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device->getLogicalDevice(), m_swapChainImageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapChain, nullptr);
}

VkFramebuffer VulkanSwapChain::createFramebuffers(VkExtent2D extent, const std::vector<VkImageView>& attachments, VkRenderPass renderPass) {
    VkFramebuffer framebuffer;

    VkFramebufferCreateInfo framebufferInfo{
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .width           = extent.width,
        .height          = extent.height,
        .layers          = 1,
    };

    VK_CHECK_RESULT(vkCreateFramebuffer(m_device->getLogicalDevice(), &framebufferInfo, nullptr, &framebuffer));
    return framebuffer;
}
VkResult VulkanSwapChain::acqureNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const {
    return vkAcquireNextImageKHR(m_device->getLogicalDevice(), m_swapChain, UINT64_MAX, semaphore, VK_NULL_HANDLE, pImageIndex);
}
VkPresentInfoKHR VulkanSwapChain::getPresentInfo(VkSemaphore *waitSemaphores, const uint32_t *imageIndex) {
    VkPresentInfoKHR presentInfo  = {
         .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
         .waitSemaphoreCount = 1,
         .pWaitSemaphores    = waitSemaphores,
         .swapchainCount     = 1,
         .pSwapchains        = &m_swapChain,
         .pImageIndices      = imageIndex,
         .pResults           = nullptr, // Optional
    };
    return presentInfo;
}

VkFormat VulkanSwapChain::getFormat() const {
    return m_swapChainImageFormat;
}
VkExtent2D VulkanSwapChain::getExtent() const {
    return m_swapChainExtent;
}
uint32_t VulkanSwapChain::getImageCount() const {
    return m_swapChainImages.size();
}
VkImageView VulkanSwapChain::getImageViewWithIdx(uint32_t idx) {
    return m_swapChainImageViews[idx];
}
} // namespace vkl
