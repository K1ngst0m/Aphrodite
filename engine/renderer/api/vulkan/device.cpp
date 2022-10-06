#include "device.h"
#include "buffer.h"
#include "commandPool.h"
#include "commandBuffer.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "renderpass.h"
#include "swapChain.h"
#include "vkInit.hpp"
#include "vkUtils.h"

namespace vkl {
uint32_t VulkanDevice::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const {
    for (uint32_t i = 0; i < _deviceInfo.memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((_deviceInfo.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                if (memTypeFound) {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound) {
        *memTypeFound = false;
        return 0;
    }

    throw std::runtime_error("Could not find a matching memory type");
}

uint32_t VulkanDevice::findQueueFamilies(VkQueueFlags queueFlags) const {
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(_deviceInfo.queueFamilyProperties.size()); i++) {
            if ((_deviceInfo.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                ((_deviceInfo.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(_deviceInfo.queueFamilyProperties.size()); i++) {
            if ((_deviceInfo.queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                ((_deviceInfo.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                ((_deviceInfo.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                return i;
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
    for (uint32_t i = 0; i < static_cast<uint32_t>(_deviceInfo.queueFamilyProperties.size()); i++) {
        if ((_deviceInfo.queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags) {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}

VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures  enabledFeatures,
                                           std::vector<const char *> enabledExtensions,
                                           void                     *pNextChain,
                                           bool                      useSwapChain,
                                           VkQueueFlags              requestedQueueTypes) {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(_deviceInfo.physicalDevice, &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    _deviceInfo.queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_deviceInfo.physicalDevice, &queueFamilyCount, _deviceInfo.queueFamilyProperties.data());

    // Desired queues need to be requested upon logical device creation
    // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
    // requests different queue types

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

    // Get queue family indices for the requested queue family types
    // Note that the indices may overlap depending on the implementation

    const float defaultQueuePriority(0.0f);

    // Graphics queue
    if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
        _queueFamilyIndices.graphics = findQueueFamilies(VK_QUEUE_GRAPHICS_BIT);
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = _queueFamilyIndices.graphics;
        queueInfo.queueCount       = 1;
        queueInfo.pQueuePriorities = &defaultQueuePriority;
        queueCreateInfos.push_back(queueInfo);
    } else {
        _queueFamilyIndices.graphics = 0;
    }

    // Dedicated compute queue
    if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
        _queueFamilyIndices.compute = findQueueFamilies(VK_QUEUE_COMPUTE_BIT);
        if (_queueFamilyIndices.compute != _queueFamilyIndices.graphics) {
            // If compute family index differs, we need an additional queue create info for the compute queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = _queueFamilyIndices.compute;
            queueInfo.queueCount       = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    } else {
        // Else we use the same queue
        _queueFamilyIndices.compute = _queueFamilyIndices.graphics;
    }

    // Dedicated transfer queue
    if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
        _queueFamilyIndices.transfer = findQueueFamilies(VK_QUEUE_TRANSFER_BIT);
        if ((_queueFamilyIndices.transfer != _queueFamilyIndices.graphics) &&
            (_queueFamilyIndices.transfer != _queueFamilyIndices.compute)) {
            // If transfer family index differs, we need an additional queue create info for the transfer queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = _queueFamilyIndices.transfer;
            queueInfo.queueCount       = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    } else {
        // Else we use the same queue
        _queueFamilyIndices.transfer = _queueFamilyIndices.graphics;
    }

    // Create the logical device representation
    std::vector<const char *> deviceExtensions(std::move(enabledExtensions));
    if (useSwapChain) {
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    VkDeviceCreateInfo deviceCreateInfo   = {};
    deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures     = &enabledFeatures;

    // If a pNext(Chain) has been passed, we need to add it to the device creation info
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    if (pNextChain) {
        physicalDeviceFeatures2.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.features  = enabledFeatures;
        physicalDeviceFeatures2.pNext     = pNextChain;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.pNext            = &physicalDeviceFeatures2;
    }

    // Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
    if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }

    if (!deviceExtensions.empty()) {
        for (const char *enabledExtension : deviceExtensions) {
            if (!extensionSupported(enabledExtension)) {
                std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
            }
        }

        deviceCreateInfo.enabledExtensionCount   = (uint32_t)deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }

    this->_deviceInfo.enabledFeatures = enabledFeatures;

    VkResult result = vkCreateDevice(_deviceInfo.physicalDevice, &deviceCreateInfo, nullptr, &_deviceInfo.logicalDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    for (auto i = 0; i < queueCreateInfos.size(); i++) {
        QueueFamily *qf = nullptr;
        if (queueCreateInfos[i].queueFamilyIndex == _queueFamilyIndices.graphics) {
            qf = &_queues[QUEUE_TYPE_GRAPHICS];
        }
        if (queueCreateInfos[i].queueFamilyIndex == _queueFamilyIndices.transfer) {
            qf = &_queues[QUEUE_TYPE_TRANSFER];
        }
        if (queueCreateInfos[i].queueFamilyIndex == _queueFamilyIndices.compute) {
            qf = &_queues[QUEUE_TYPE_COMPUTE];
        }

        assert(qf);

        for (auto j = 0; j < queueCreateInfos[i].queueCount; j++) {
            VkQueue queue = VK_NULL_HANDLE;
            vkGetDeviceQueue(getLogicalDevice(), i, j, &queue);
            if (queue) {
                qf->push_back(queue);
            }
        }
    }

    return result;
}

VkResult VulkanDevice::createCommandPool(VulkanCommandPool **ppPool, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) {
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = queueFamilyIndex;
    cmdPoolInfo.flags                   = createFlags;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    auto result = vkCreateCommandPool(_deviceInfo.logicalDevice, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS){
        return result;
    }

    *ppPool = VulkanCommandPool::Create(this, queueFamilyIndex, cmdPool);
    return VK_SUCCESS;
}

bool VulkanDevice::extensionSupported(std::string_view extension) const {
    return (std::find(_deviceInfo.supportedExtensions.begin(), _deviceInfo.supportedExtensions.end(), extension) != _deviceInfo.supportedExtensions.end());
}

VkFormat VulkanDevice::getDepthFormat() const {
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                           VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_deviceInfo.physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    assert("failed to find supported format!");
    return {};
}

VkResult VulkanDevice::createImageView(ImageViewCreateInfo *pCreateInfo, VulkanImageView **ppImageView, VulkanImage *pImage) {
    // Create a new Vulkan image view.
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.image                 = pImage->getHandle();
    createInfo.viewType              = static_cast<VkImageViewType>(pCreateInfo->viewType);
    createInfo.format                = static_cast<VkFormat>(pCreateInfo->format);
    memcpy(&createInfo.components, &pCreateInfo->components, sizeof(VkComponentMapping));
    createInfo.subresourceRange.aspectMask     = vkl::utils::getImageAspectFlags(static_cast<VkFormat>(createInfo.format));
    createInfo.subresourceRange.baseMipLevel   = pCreateInfo->subresourceRange.baseMipLevel;
    createInfo.subresourceRange.levelCount     = pCreateInfo->subresourceRange.levelCount;
    createInfo.subresourceRange.baseArrayLayer = pCreateInfo->subresourceRange.baseArrayLayer;
    createInfo.subresourceRange.layerCount     = pCreateInfo->subresourceRange.layerCount;
    VkImageView handle                         = VK_NULL_HANDLE;
    auto        result                         = vkCreateImageView(pImage->getDevice()->getLogicalDevice(), &createInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    *ppImageView = VulkanImageView::createFromHandle(pCreateInfo, pImage, handle);

    return VK_SUCCESS;
}
void VulkanDevice::copyBufferToImage(VkCommandBuffer commandBuffer, VulkanBuffer *buffer, VulkanImage *image) {
    VkBufferImageCopy region{
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
    };

    region.imageSubresource = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image->getWidth(), image->getHeight(), 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer->getHandle(), image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer, QueueFlags flags) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = vkl::init::submitInfo(&commandBuffer);

    vkQueueSubmit(getQueueByFlags(flags), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(getQueueByFlags(flags));

    vkFreeCommandBuffers(_deviceInfo.logicalDevice, getCommandPoolWithQueue(flags)->getHandle(), 1, &commandBuffer);
}
VkCommandBuffer VulkanDevice::beginSingleTimeCommands(QueueFlags flags) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = getCommandPoolWithQueue(flags)->getHandle(),
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_deviceInfo.logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void VulkanDevice::copyBuffer(VkCommandBuffer commandBuffer, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size) {
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);
}
void VulkanDevice::transitionImageLayout(VkCommandBuffer commandBuffer, VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    VkImageMemoryBarrier imageMemoryBarrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->getHandle(),
    };

    const auto &imageCreateInfo         = image->getCreateInfo();
    imageMemoryBarrier.subresourceRange = {
        .aspectMask     = vkl::utils::getImageAspectFlags(static_cast<VkFormat>(imageCreateInfo.format)),
        .baseMipLevel   = 0,
        .levelCount     = imageCreateInfo.mipLevels,
        .baseArrayLayer = 0,
        .layerCount     = imageCreateInfo.arrayLayers,
    };

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0) {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    vkCmdPipelineBarrier(commandBuffer,
                         srcStageMask,
                         dstStageMask,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier);
}

VkResult VulkanDevice::createBuffer(BufferCreateInfo *pCreateInfo, VulkanBuffer **ppBuffer, void *data) {
    VkBuffer       buffer;
    VkDeviceMemory memory;
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = pCreateInfo->size,
        .usage       = pCreateInfo->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK_RESULT(vkCreateBuffer(_deviceInfo.logicalDevice, &bufferInfo, nullptr, &buffer));

    // create memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_deviceInfo.logicalDevice, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, pCreateInfo->property),
    };
    VK_CHECK_RESULT(vkAllocateMemory(_deviceInfo.logicalDevice, &allocInfo, nullptr, &memory));

    *ppBuffer = VulkanBuffer::CreateFromHandle(this, pCreateInfo, buffer, memory);

    // bind buffer and memory
    VkResult result = (*ppBuffer)->bind();
    VK_CHECK_RESULT(result);

    if (data) {
        (*ppBuffer)->map();
        (*ppBuffer)->copyTo(data, (*ppBuffer)->getSize());
        (*ppBuffer)->unmap();
    }

    return result;
}
VkResult VulkanDevice::createImage(ImageCreateInfo *pCreateInfo, VulkanImage **ppImage) {
    VkImage        image;
    VkDeviceMemory memory;

    VkImageCreateInfo imageCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = static_cast<VkFormat>(pCreateInfo->format),
        .mipLevels     = pCreateInfo->mipLevels,
        .arrayLayers   = pCreateInfo->layerCount,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = static_cast<VkImageTiling>(pCreateInfo->tiling),
        .usage         = pCreateInfo->usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    imageCreateInfo.extent.width  = pCreateInfo->extent.width;
    imageCreateInfo.extent.height = pCreateInfo->extent.height;
    imageCreateInfo.extent.depth  = pCreateInfo->extent.depth;

    if (vkCreateImage(_deviceInfo.logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_deviceInfo.logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, pCreateInfo->property),
    };

    if (vkAllocateMemory(_deviceInfo.logicalDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    *ppImage = VulkanImage::CreateFromHandle(this, pCreateInfo, image, memory);

    if ((*ppImage)->getMemory() != VK_NULL_HANDLE) {
        auto result = (*ppImage)->bind();
        return result;
    }

    return VK_SUCCESS;
}

void VulkanDevice::destroy() {
    if (_drawCommandPool) {
        destroyCommandPool(_drawCommandPool);
    }
    if (_transferCommandPool) {
        destroyCommandPool(_transferCommandPool);
    }
    if (_computeCommandPool) {
        destroyCommandPool(_computeCommandPool);
    }

    if (_deviceInfo.logicalDevice) {
        vkDestroyDevice(_deviceInfo.logicalDevice, nullptr);
    }
}

uint32_t &VulkanDevice::getQueueFamilyIndices(QueueFlags type) {
    switch (type) {
    case QUEUE_TYPE_COMPUTE:
        return _queueFamilyIndices.compute;
    case QUEUE_TYPE_GRAPHICS:
        return _queueFamilyIndices.graphics;
    case QUEUE_TYPE_TRANSFER:
        return _queueFamilyIndices.transfer;
    case QUEUE_TYPE_PRESENT:
        return _queueFamilyIndices.present;
    default:
        return _queueFamilyIndices.graphics;
    }
}

void VulkanDevice::init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures features, const std::vector<const char *> &extension) {
    assert(physicalDevice);
    this->_deviceInfo.physicalDevice = physicalDevice;

    vkGetPhysicalDeviceProperties(physicalDevice, &_deviceInfo.properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &_deviceInfo.features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_deviceInfo.memoryProperties);

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) ==
            VK_SUCCESS) {
            for (auto ext : extensions) {
                _deviceInfo.supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }

    createLogicalDevice(features, extension, nullptr);

    createCommandPool(&_drawCommandPool, _queueFamilyIndices.graphics);
    createCommandPool(&_transferCommandPool, _queueFamilyIndices.transfer);
    createCommandPool(&_computeCommandPool, _queueFamilyIndices.compute);
}

VkDevice VulkanDevice::getLogicalDevice() const {
    return _deviceInfo.logicalDevice;
}
VkPhysicalDevice VulkanDevice::getPhysicalDevice() const {
    return _deviceInfo.physicalDevice;
}
void VulkanDevice::allocateCommandBuffers(VkCommandBuffer *cmdbuffer, uint32_t count, QueueFlags flags) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = getCommandPoolWithQueue(flags)->getHandle(),
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    VK_CHECK_RESULT(vkAllocateCommandBuffers(getLogicalDevice(), &allocInfo, cmdbuffer));
}
VkPhysicalDeviceFeatures &VulkanDevice::getDeviceEnabledFeatures() {
    return _deviceInfo.enabledFeatures;
}
VkPhysicalDeviceProperties &VulkanDevice::getDeviceProperties() {
    return _deviceInfo.properties;
}

VkResult VulkanDevice::createRenderPass(RenderPassCreateInfo                       *createInfo,
                                        VulkanRenderPass                          **ppRenderPass,
                                        const std::vector<VkAttachmentDescription> &colorAttachments,
                                        const VkAttachmentDescription              &depthAttachment) {

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference>   colorAttachmentRefs;

    for (uint32_t idx = 0; idx < colorAttachments.size(); idx++) {
        attachments.push_back(colorAttachments[idx]);
        VkAttachmentReference ref{};
        ref.attachment = idx;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(ref);
    }

    attachments.push_back(depthAttachment);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = static_cast<uint32_t>(colorAttachments.size());
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpassDescription.pColorAttachments       = colorAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = 1,
        .pSubpasses      = &subpassDescription,
        .dependencyCount = dependencies.size(),
        .pDependencies   = dependencies.data(),
    };

    VkRenderPass renderpass;
    auto         result = vkCreateRenderPass(_deviceInfo.logicalDevice, &renderPassInfo, nullptr, &renderpass);

    if (result != VK_SUCCESS) {
        return result;
    }

    *ppRenderPass = new VulkanRenderPass(renderpass, colorAttachmentRefs.size());

    return VK_SUCCESS;
}

VkResult VulkanDevice::createFramebuffers(FramebufferCreateInfo *pCreateInfo,
                                          VulkanFramebuffer    **ppFramebuffer,
                                          uint32_t               attachmentCount,
                                          VulkanImageView      **pAttachments) {
    return VulkanFramebuffer::Create(this, pCreateInfo, ppFramebuffer, attachmentCount, pAttachments);
}

void VulkanDevice::destroyBuffer(VulkanBuffer *pBuffer) {
    if (pBuffer->getMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(_deviceInfo.logicalDevice, pBuffer->getMemory(), nullptr);
    }
    vkDestroyBuffer(_deviceInfo.logicalDevice, pBuffer->getHandle(), nullptr);
    delete pBuffer;
}
void VulkanDevice::destroyImage(VulkanImage *pImage) {
    if (pImage->getMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(_deviceInfo.logicalDevice, pImage->getMemory(), nullptr);
    }
    vkDestroyImage(_deviceInfo.logicalDevice, pImage->getHandle(), nullptr);
    delete pImage;
}
void VulkanDevice::destroyImageView(VulkanImageView *pImageView) {
    vkDestroyImageView(_deviceInfo.logicalDevice, pImageView->getHandle(), nullptr);
    delete pImageView;
}
void VulkanDevice::destoryRenderPass(VulkanRenderPass *pRenderpass) {
    vkDestroyRenderPass(_deviceInfo.logicalDevice, pRenderpass->getHandle(), nullptr);
    delete pRenderpass;
}
void VulkanDevice::destroyFramebuffers(VulkanFramebuffer *pFramebuffer) {
    delete pFramebuffer;
}
void VulkanDevice::copyImage(VkCommandBuffer commandBuffer,
                             VulkanImage    *srcImage,
                             VulkanImage    *dstImage) {
    // Copy region for transfer from framebuffer to cube face
    VkImageCopy copyRegion = {};
    copyRegion.srcOffset   = {0, 0, 0};
    copyRegion.dstOffset   = {0, 0, 0};

    VkImageSubresourceLayers subresourceLayers{
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    copyRegion.srcSubresource = subresourceLayers;
    copyRegion.dstSubresource = subresourceLayers;
    copyRegion.extent.width   = srcImage->getWidth();
    copyRegion.extent.height  = srcImage->getHeight();
    copyRegion.extent.depth   = 1;

    vkCmdCopyImage(commandBuffer,
                   srcImage->getHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage->getHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &copyRegion);
}
VkResult VulkanDevice::createSwapchain(VkSurfaceKHR surface, VulkanSwapChain **ppSwapchain, WindowData *data) {
    VulkanSwapChain *instance = new VulkanSwapChain;
    instance->create(this, surface, data);

    // get present queue family
    {
        VkBool32                presentSupport = false;
        std::optional<uint32_t> presentQueueFamilyIndices;
        uint32_t                i = 0;

        for (const auto &queueFamily : _deviceInfo.queueFamilyProperties) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_deviceInfo.physicalDevice, i, surface, &presentSupport);
            if (presentSupport) {
                presentQueueFamilyIndices = i;
                break;
            }
            i++;
        }
        assert(presentQueueFamilyIndices.has_value());

        _queueFamilyIndices.present = presentQueueFamilyIndices.value();

        // TODO if present queue familiy not the same as graphics
        if (_queueFamilyIndices.present == _queueFamilyIndices.graphics) {
            _queues[QUEUE_TYPE_PRESENT] = _queues[QUEUE_TYPE_GRAPHICS];
        } else {
            assert("present queue familiy not the same as graphics!");
        }
    }

    *ppSwapchain = instance;

    // TODO
    return VK_SUCCESS;
}
void VulkanDevice::destroySwapchain(VulkanSwapChain *pSwapchain) {
    vkDestroySwapchainKHR(getLogicalDevice(), pSwapchain->getHandle(), nullptr);
    delete pSwapchain;
}
VkQueue VulkanDevice::getQueueByFlags(QueueFlags queueFlags, uint32_t queueIndex) {
    return _queues[queueFlags][queueIndex];
}
void VulkanDevice::waitIdle() {
    vkDeviceWaitIdle(getLogicalDevice());
}

VulkanCommandPool* VulkanDevice::getCommandPoolWithQueue(QueueFlags type) {
    switch (type) {
    case QUEUE_TYPE_COMPUTE:
        return _computeCommandPool;
    case QUEUE_TYPE_GRAPHICS:
    case QUEUE_TYPE_PRESENT:
        return _drawCommandPool;
    case QUEUE_TYPE_TRANSFER:
        return _transferCommandPool;
    default:
        return _drawCommandPool;
    }
}

void VulkanDevice::immediateSubmit(QueueFlags flags, std::function<void(VkCommandBuffer cmd)> &&function) {
    VkCommandBuffer cmd = beginSingleTimeCommands(flags);
    function(cmd);
    endSingleTimeCommands(cmd, flags);
}
void VulkanDevice::destroyCommandPool(VulkanCommandPool *pPool) {
    vkDestroyCommandPool(getLogicalDevice(), pPool->getHandle(), nullptr);
    delete pPool;
}
} // namespace vkl
