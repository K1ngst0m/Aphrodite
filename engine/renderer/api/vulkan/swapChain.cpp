#include "swapChain.h"
#include "buffer.h"
#include "image.h"
#include "imageView.h"

namespace vkl {
SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void VulkanSwapChain::create(const std::shared_ptr<VulkanDevice> &device, VkSurfaceKHR surface, WindowData *data) {
    _device  = device;
    _surface = surface;
    allocate(data);
}

void VulkanSwapChain::cleanup() {
    vkDestroySwapchainKHR(_device->getLogicalDevice(), _handle, nullptr);
}

VkResult VulkanSwapChain::acqureNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const {
    return vkAcquireNextImageKHR(_device->getLogicalDevice(), _handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, pImageIndex);
}
VkPresentInfoKHR VulkanSwapChain::getPresentInfo(VkSemaphore *waitSemaphores, const uint32_t *imageIndex) {
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = waitSemaphores,
        .swapchainCount     = 1,
        .pSwapchains        = &_handle,
        .pImageIndices      = imageIndex,
        .pResults           = nullptr, // Optional
    };
    return presentInfo;
}

void VulkanSwapChain::allocate(WindowData *data) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = vkl::utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR   presentMode   = vkl::utils::chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D         extent        = vkl::utils::chooseSwapExtent(swapChainSupport.capabilities, data->window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = _surface,
        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform     = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = presentMode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    std::array<uint32_t, 2> queueFamilyIndices = {_device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS),
                                                  _device->GetQueueFamilyIndices(DeviceQueueType::PRESENT)};

    if (_device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS) != _device->GetQueueFamilyIndices(DeviceQueueType::PRESENT)) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(_device->getLogicalDevice(), &createInfo, nullptr, &_handle));

    vkGetSwapchainImagesKHR(_device->getLogicalDevice(), _handle, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(_device->getLogicalDevice(), _handle, &imageCount, images.data());

    // Create an Image class instances to wrap swapchain image handles.
    for (auto handle : images) {
        ImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.imageType       = IMAGE_TYPE_2D;
        imageCreateInfo.format          = static_cast<Format>(_imageFormat);
        imageCreateInfo.extent          = {extent.width, extent.height, 1};
        imageCreateInfo.mipLevels       = 1;
        imageCreateInfo.arrayLayers     = 1;
        imageCreateInfo.samples         = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling          = IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage           = IMAGE_USAGE_TRANSFER_DST_BIT;

        auto image = VulkanImage::createFromHandle(_device.get(), &imageCreateInfo, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, handle);

        // Transition image to default layout.
        VkQueue queue;
        vkGetDeviceQueue(_device->getLogicalDevice(), _device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS), 0, &queue);
        _device->transitionImageLayout(queue, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        _images.push_back(image);
    }

    _imageFormat = surfaceFormat.format;
    _extent      = extent;
}

VkFormat VulkanSwapChain::getFormat() const {
    return _imageFormat;
}
VkExtent2D VulkanSwapChain::getExtent() const {
    return _extent;
}
uint32_t VulkanSwapChain::getImageCount() const {
    return _images.size();
}
VulkanImage *VulkanSwapChain::getImage(uint32_t idx) const {
    return _images[idx];
}
} // namespace vkl
