#include "device.h"
#include "buffer.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "renderpass.h"
#include "vkInit.hpp"
#include "vkUtils.h"

namespace vkl {
/**
 * Default constructor
 *
 * @param physicalDevice Physical device that is to be used
 */
VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice) {
    assert(physicalDevice);
    this->_deviceInfo.physicalDevice = physicalDevice;

    vkGetPhysicalDeviceProperties(physicalDevice, &_deviceInfo.properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &_deviceInfo.features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_deviceInfo.memoryProperties);

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    _deviceInfo.queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, _deviceInfo.queueFamilyProperties.data());

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
}

/**
 * Get the index of a memory type that has all the requested property bits set
 *
 * @param typeBits Bit mask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
 * @param properties Bit mask of properties for the memory type to request
 * @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
 *
 * @return Index of the requested memory type
 *
 * @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
 */
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

/**
 * Get the index of a queue family that supports the requested queue flags
 * SRS - support VkQueueFlags parameter for requesting multiple flags vs. VkQueueFlagBits for a single flag only
 *
 * @param queueFlags Queue flags to find a queue family index for
 *
 * @return Index of the queue family index that matches the flags
 *
 * @throw Throws an exception if no queue family index could be found that supports the requested flags
 */
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

/**
 * Create the logical device based on the assigned physical device, also gets default queue family indices
 *
 * @param enabledFeatures Can be used to enable certain features upon device creation
 * @param pNextChain Optional chain of pointer to extension structures
 * @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
 * @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
 *
 * @return VkResult of the device creation call
 */
VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures  enabledFeatures,
                                           std::vector<const char *> enabledExtensions, void *pNextChain,
                                           bool useSwapChain, VkQueueFlags requestedQueueTypes) {
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
        _enableDebugMarkers = true;
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

    // Create a default command pool for graphics command buffers
    _commandPool = createCommandPool(_queueFamilyIndices.graphics);

    return result;
}

/**
 * Create a command pool for allocation command buffers from
 *
 * @param queueFamilyIndex Family index of the queue to create the command pool for
 * @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
 *
 * @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
 *
 * @return A handle to the created command buffer
 */
VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) const {
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = queueFamilyIndex;
    cmdPoolInfo.flags                   = createFlags;
    VkCommandPool cmdPool;
    VK_CHECK_RESULT(vkCreateCommandPool(_deviceInfo.logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
    return cmdPool;
}

/**
 * Finish command buffer recording and submit it to a queue
 *
 * @param commandBuffer Command buffer to flush
 * @param queue Queue to submit the command buffer to
 * @param pool Command pool on which the command buffer has been created
 * @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
 *
 * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
 * @note Uses a fence to ensure command buffer has finished executing
 */
void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free) const {
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = vkl::init::submitInfo(&commandBuffer);

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = vkl::init::fenceCreateInfo(VK_FLAGS_NONE);
    VkFence           fence;
    VK_CHECK_RESULT(vkCreateFence(_deviceInfo.logicalDevice, &fenceInfo, nullptr, &fence));
    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(_deviceInfo.logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(_deviceInfo.logicalDevice, fence, nullptr);
    if (free) {
        vkFreeCommandBuffers(_deviceInfo.logicalDevice, pool, 1, &commandBuffer);
    }
}

void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free) const {
    return flushCommandBuffer(commandBuffer, queue, _commandPool, free);
}

/**
 * Check if an extension is supported by the (physical device)
 *
 * @param extension Name of the extension to check
 *
 * @return True if the extension is supported (present in the list read at device creation time)
 */
bool VulkanDevice::extensionSupported(std::string_view extension) const {
    return (std::find(_deviceInfo.supportedExtensions.begin(), _deviceInfo.supportedExtensions.end(), extension) != _deviceInfo.supportedExtensions.end());
}

/**
 * Select the best-fit depth format for this device from a list of possible depth (and stencil) formats
 * @return The depth format that best fits for the current device
 *
 * @throw Throws an exception if no depth format fits the requirements
 */
VkFormat VulkanDevice::findDepthFormat() const {
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
void VulkanDevice::copyBufferToImage(VkQueue queue, VulkanBuffer *buffer, VulkanImage *image) {
    VkCommandBuffer   commandBuffer = beginSingleTimeCommands();
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

    endSingleTimeCommands(commandBuffer, queue);
}
void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = vkl::init::submitInfo(&commandBuffer);

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(_deviceInfo.logicalDevice, _commandPool, 1, &commandBuffer);
}
VkCommandBuffer VulkanDevice::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
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
void VulkanDevice::copyBuffer(VkQueue queue, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, queue);
}
void VulkanDevice::transitionImageLayout(VkQueue queue, VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer      commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = 0,
        .dstAccessMask       = 0,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->getHandle(),
    };

    const auto &imageCreateInfo = image->getCreateInfo();
    barrier.subresourceRange    = {
           .aspectMask     = vkl::utils::getImageAspectFlags(static_cast<VkFormat>(imageCreateInfo.format)),
           .baseMipLevel   = 0,
           .levelCount     = imageCreateInfo.mipLevels,
           .baseArrayLayer = 0,
           .layerCount     = imageCreateInfo.arrayLayers,
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer, queue);
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

    *ppBuffer = VulkanBuffer::createFromHandle(this, pCreateInfo, buffer, memory);

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

    VkImageLayout defaultLayout = utils::getDefaultImageLayoutFromUsage(pCreateInfo->usage);

    *ppImage = VulkanImage::createFromHandle(this, pCreateInfo, defaultLayout, image, memory);
    if ((*ppImage)->getMemory() != VK_NULL_HANDLE) {
        return (*ppImage)->bind();
    }
    return VK_SUCCESS;
}

void VulkanDevice::destroy() const {
    if (_commandPool) {
        vkDestroyCommandPool(_deviceInfo.logicalDevice, _commandPool, nullptr);
    }
    if (_deviceInfo.logicalDevice) {
        vkDestroyDevice(_deviceInfo.logicalDevice, nullptr);
    }
}

uint32_t &VulkanDevice::GetQueueFamilyIndices(DeviceQueueType type) {
    switch (type) {
    case DeviceQueueType::COMPUTE:
        return _queueFamilyIndices.compute;
    case DeviceQueueType::GRAPHICS:
        return _queueFamilyIndices.graphics;
    case DeviceQueueType::TRANSFER:
        return _queueFamilyIndices.transfer;
    case DeviceQueueType::PRESENT:
        return _queueFamilyIndices.present;
    }
    return _queueFamilyIndices.graphics;
}
void VulkanDevice::init(VkSurfaceKHR surface, VkPhysicalDeviceFeatures features, const std::vector<const char *> &extension) {
    createLogicalDevice(features, extension, nullptr);
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
}
VkDevice VulkanDevice::getLogicalDevice() {
    return _deviceInfo.logicalDevice;
}
VkPhysicalDevice VulkanDevice::getPhysicalDevice() {
    return _deviceInfo.physicalDevice;
}
void VulkanDevice::allocateCommandBuffers(VkCommandBuffer *cmdbuffer, uint32_t count) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
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
    return VulkanFramebuffer::create(this, pCreateInfo, ppFramebuffer, attachmentCount, pAttachments);
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
void VulkanDevice::copyImage(VkQueue      queue,
                             VulkanImage *srcImage,
                             VulkanImage *dstImage) {
    auto          command   = beginSingleTimeCommands();
    VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    VkImageSubresourceLayers subresourceLayers{};
    subresourceLayers.layerCount     = 1;
    subresourceLayers.mipLevel       = 0;
    subresourceLayers.baseArrayLayer = 0;
    subresourceLayers.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageCopy region{};
    region.dstOffset      = {0, 0, 0};
    region.srcOffset      = {0, 0, 0};
    region.dstSubresource = subresourceLayers;
    region.srcSubresource = subresourceLayers;
    region.extent         = {srcImage->getExtent().width, srcImage->getExtent().height, srcImage->getExtent().depth};

    vkCmdCopyImage(command, srcImage->getHandle(), srcLayout, dstImage->getHandle(), dstLayout, 1, &region);

    endSingleTimeCommands(command, queue);
}
} // namespace vkl
