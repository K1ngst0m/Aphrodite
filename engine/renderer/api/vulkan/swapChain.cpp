#include "swapChain.h"
#include "buffer.h"
#include "image.h"
#include "imageView.h"

namespace vkl {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR _surface, VkPhysicalDevice device) {
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

void VulkanSwapChain::create(VulkanDevice *device, VkSurfaceKHR surface, WindowData *data) {
    _device  = device;
    _surface = surface;
    allocate(data);
}

void VulkanSwapChain::cleanup() {
    vkDestroySwapchainKHR(_device->getHandle(), _handle, nullptr);
}

VkResult VulkanSwapChain::acqureNextImage(VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) const {
    return vkAcquireNextImageKHR(_device->getHandle(), _handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, pImageIndex);
}

void VulkanSwapChain::allocate(WindowData *data) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_surface, _device->getPhysicalDevice()->getHandle());

    VkSurfaceFormatKHR surfaceFormat = vkl::utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR   presentMode   = vkl::utils::chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D         extent        = vkl::utils::chooseSwapExtent(swapChainSupport.capabilities, data->window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = _surface,
        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform     = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = presentMode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    std::array<uint32_t, 2> queueFamilyIndices = {_device->getPhysicalDevice()->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS),
                                                  _device->getPhysicalDevice()->getQueueFamilyIndices(QUEUE_TYPE_PRESENT)};

    if (_device->getPhysicalDevice()->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS) != _device->getPhysicalDevice()->getQueueFamilyIndices(QUEUE_TYPE_PRESENT)) {
        swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
    } else {
        swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapChainCreateInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(_device->getHandle(), &swapChainCreateInfo, nullptr, &_handle));

    vkGetSwapchainImagesKHR(_device->getHandle(), _handle, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(_device->getHandle(), _handle, &imageCount, images.data());

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
        imageCreateInfo.usage           = swapChainCreateInfo.imageUsage;

        auto image = VulkanImage::CreateFromHandle(_device, &imageCreateInfo, handle);

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
