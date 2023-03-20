#include "swapChain.h"
#include "buffer.h"
#include "image.h"
#include "imageView.h"

namespace vkl
{
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for(const auto &availableFormat : availableFormats)
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
           availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for(const auto &availablePresentMode : availablePresentModes)
    {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR _surface, VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if(formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if(presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VulkanSwapChain *VulkanSwapChain::Create(VulkanDevice *device, VkSurfaceKHR surface, WindowData *data)
{
    auto instance = new VulkanSwapChain;
    instance->_device = device;
    instance->_surface = surface;
    instance->allocateImages(data);
    return instance;
}

VkResult VulkanSwapChain::acquireNextImage(uint32_t *pImageIndex, VkSemaphore semaphore,
                                           VkFence fence) const
{
    return vkAcquireNextImageKHR(_device->getHandle(), _handle, UINT64_MAX, semaphore, fence,
                                 pImageIndex);
}
void VulkanSwapChain::allocateImages(WindowData *data)
{
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(_surface, _device->getPhysicalDevice()->getHandle());
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, data->window);

    _imageColorSpace = surfaceFormat.colorSpace;
    _imageFormat = surfaceFormat.format;
    _extent = extent;
    _imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 &&
       _imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        _imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = _surface,
        .minImageCount = _imageCount,
        .imageFormat = _imageFormat,
        .imageColorSpace = _imageColorSpace,
        .imageExtent = _extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;      // Optional
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;  // Optional

    VK_CHECK_RESULT(vkCreateSwapchainKHR(_device->getHandle(), &swapChainCreateInfo, nullptr, &_handle));

    vkGetSwapchainImagesKHR(_device->getHandle(), _handle, &_imageCount, nullptr);
    _imageCount = std::max(_imageCount, MAX_SWAPCHAIN_IMAGE_COUNT);
    std::vector<VkImage> images(_imageCount);
    vkGetSwapchainImagesKHR(_device->getHandle(), _handle, &_imageCount, images.data());

    // Create an Image class instances to wrap swapchain image handles.
    for(auto handle : images)
    {
        ImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.imageType = IMAGE_TYPE_2D;
        imageCreateInfo.format = static_cast<Format>(_imageFormat);
        imageCreateInfo.extent = { _extent.width, _extent.height, 1 };
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = swapChainCreateInfo.imageUsage;

        auto image = new VulkanImage(_device, imageCreateInfo, handle);

        _images.push_back(image);
    }
}
void VulkanSwapChain::cleanupImages()
{
}
}  // namespace vkl
