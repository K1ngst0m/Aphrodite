#include "physicalDevice.h"

namespace vkl {

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle)
    : m_instance(instance) {
    _handle = handle;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    _queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, _queueFamilyProperties.data());
    vkGetPhysicalDeviceProperties(_handle, &_properties);
    vkGetPhysicalDeviceFeatures(_handle, &_features);
    vkGetPhysicalDeviceMemoryProperties(_handle, &_memoryProperties);

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(_handle, nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(_handle, nullptr, &extCount, &extensions.front()) ==
            VK_SUCCESS) {
            for (auto ext : extensions) {
                _supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }

    _queueFamilyIndices.graphics = findQueueFamilies(VK_QUEUE_GRAPHICS_BIT);
    _queueFamilyIndices.compute  = findQueueFamilies(VK_QUEUE_COMPUTE_BIT);
    _queueFamilyIndices.transfer = findQueueFamilies(VK_QUEUE_TRANSFER_BIT);
    _queueFamilyIndices.present = _queueFamilyIndices.graphics;
}

const VulkanInstance *VulkanPhysicalDevice::getInstance() const {
    return m_instance;
}
const VkPhysicalDeviceProperties &VulkanPhysicalDevice::getDeviceProperties() {
    return _properties;
}
const VkPhysicalDeviceFeatures &VulkanPhysicalDevice::getDeviceFeatures() {
    return _features;
}
const VkPhysicalDeviceMemoryProperties &VulkanPhysicalDevice::getMemoryProperties() {
    return _memoryProperties;
}
const std::vector<std::string> &VulkanPhysicalDevice::getDeviceSupportedExtensions() {
    return _supportedExtensions;
}
const std::vector<VkQueueFamilyProperties> &VulkanPhysicalDevice::getQueueFamilyProperties() {
    return _queueFamilyProperties;
}

uint32_t VulkanPhysicalDevice::findQueueFamilies(VkQueueFlags queueFlags) const {
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++) {
            if ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++) {
            if ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                return i;
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
    for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++) {
        if ((_queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags) {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}

bool VulkanPhysicalDevice::isExtensionSupported(std::string_view extension) const {
    return (std::find(_supportedExtensions.begin(), _supportedExtensions.end(), extension) != _supportedExtensions.end());
}

uint32_t VulkanPhysicalDevice::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const {
    for (uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
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
VkFormat VulkanPhysicalDevice::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                                   VkImageTiling                tiling,
                                                   VkFormatFeatureFlags         features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_handle, format, &props);
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
uint32_t VulkanPhysicalDevice::getQueueFamilyIndices(QueueFamilyType flags) {
    switch (flags) {
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
} // namespace vkl
