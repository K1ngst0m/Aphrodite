#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "buffer.h"
#include "texture.h"
#include "vkUtils.h"

#include "renderer/device.h"

namespace vkl {
enum class DeviceQueueType {
    COMPUTE,
    GRAPHICS,
    TRANSFER,
    PRESENT,
};

class VulkanDevice : public GraphicsDevice {
public:
    explicit VulkanDevice(VkPhysicalDevice physicalDevice);
    ~VulkanDevice() = default;

    void create(VkSurfaceKHR surface, VkPhysicalDeviceFeatures features, const std::vector<const char *> &extension);
    void destroy() const;

public:
    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    VkResult       createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer &buffer, void *data = nullptr) const;
    VkImageView    createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) const;
    VkResult       createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, vkl::VulkanTexture &texture, uint32_t miplevels = 1, uint32_t layerCount = 1) const;

    void transitionImageLayout(VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBuffer(VkQueue queue, vkl::VulkanBuffer srcBuffer, vkl::VulkanBuffer dstBuffer, VkDeviceSize size);
    void copyBuffer(VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkQueue queue, vkl::VulkanBuffer buffer, vkl::VulkanTexture texture, uint32_t width, uint32_t height);
    void copyBufferToImage(VkQueue queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkFramebuffer createFramebuffers(VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);

public:
    void            allocateCommandBuffers(VkCommandBuffer *cmdbuffer, uint32_t count);
    VkCommandPool   createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const;
    void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true) const;
    void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true) const;

public:
    VkPhysicalDevice            getPhysicalDevice();
    VkDevice                    getLogicalDevice();
    VkPhysicalDeviceFeatures   &getDeviceEnabledFeatures();
    VkPhysicalDeviceProperties &getDeviceProperties();
    uint32_t                   &GetQueueFamilyIndices(DeviceQueueType type);
    VkFormat                    findDepthFormat() const;

private:
    bool     extensionSupported(std::string_view extension) const;
    VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    uint32_t findQueueFamilies(VkQueueFlags queueFlags) const;
    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

private:
    struct {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
        uint32_t present;
    } queueFamilyIndices;

    VkPhysicalDevice                     physicalDevice;
    VkPhysicalDeviceProperties           properties;
    VkPhysicalDeviceFeatures             features;
    VkPhysicalDeviceFeatures             enabledFeatures;
    VkPhysicalDeviceMemoryProperties     memoryProperties;
    std::vector<std::string>             supportedExtensions;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;

    VkDevice logicalDevice;

    VkCommandPool commandPool        = VK_NULL_HANDLE;
    bool          enableDebugMarkers = true;
};

} // namespace vkl

#endif // VKLDEVICE_H_
