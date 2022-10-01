#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "renderer/device.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl {

enum class DeviceQueueType {
    COMPUTE,
    GRAPHICS,
    TRANSFER,
    PRESENT,
};

class VulkanBuffer;
class VulkanImage;
class VulkanImageView;
class VulkanSampler;

class VulkanDevice : public GraphicsDevice {
public:
    explicit VulkanDevice(VkPhysicalDevice physicalDevice);
    ~VulkanDevice() = default;

    void create(VkSurfaceKHR surface, VkPhysicalDeviceFeatures features, const std::vector<const char *> &extension);
    void destroy() const;

public:
    VkResult       createBuffer(BufferCreateInfo *createInfo, VulkanBuffer *buffer, void *data = nullptr);
    VkResult       createImage(ImageCreateInfo *pCreateInfo, VulkanImage *pImage);
    VkResult       createImageView(ImageViewCreateInfo *pCreateInfo, VulkanImageView *pImageView, VulkanImage *pImage) const;
    VkShaderModule createShaderModule(const std::vector<char> &code) const;

    void transitionImageLayout(VkQueue queue, VulkanImage *image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBuffer(VkQueue queue, VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkQueue queue, VulkanBuffer *buffer, VulkanImage *image);

    VkFramebuffer createFramebuffers(VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);
    VkRenderPass  createRenderPass(const std::vector<VkAttachmentDescription> &colorAttachments, VkAttachmentDescription &depthAttachment);

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
