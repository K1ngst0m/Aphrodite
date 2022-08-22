#ifndef VKLDEVICE_H_
#define VKLDEVICE_H_

#include "vklBuffer.h"
#include "vklTexture.h"
#include "vklMesh.h"
#include "vklUtils.h"

namespace vkl
{

struct Device {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceFeatures enabledFeatures;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<std::string> supportedExtensions;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;

    VkDevice logicalDevice;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    bool enableDebugMarkers = true;

    struct {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
        uint32_t present;
    } queueFamilyIndices;

    explicit Device(VkPhysicalDevice physicalDevice);

    ~Device();

    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    std::vector<const char *> getRequiredExtensions();
    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                            VkBool32 *memTypeFound = nullptr) const;
    uint32_t findQueueFamilies(VkQueueFlags queueFlags) const;
    VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures,
                                 std::vector<const char *> enabledExtensions,
                                 void *pNextChain, bool useSwapChain = true,
                                 VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer &buffer);
    void setupMesh(vkl::Mesh &mesh, VkQueue transferQueue = VK_NULL_HANDLE);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) const;
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, vkl::Texture& texture) const;
    void transitionImageLayout(VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBuffer(VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkQueue queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const;
    void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true) const;
    void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true) const;

    bool extensionSupported(std::string_view extension) const;
    VkFormat findDepthFormat() const;
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};

}

#endif // VKLDEVICE_H_
