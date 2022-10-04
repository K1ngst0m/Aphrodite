#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "renderer/device.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl {
class VulkanBuffer;
class VulkanBufferView;
class VulkanImage;
class VulkanImageView;
class VulkanSampler;
class VulkanFramebuffer;
class VulkanRenderPass;
class VulkanSwapChain;
class WindowData;
struct RenderPassCreateInfo;

using QueueFamily = std::vector<VkQueue>;

enum QueueFlags {
    QUEUE_TYPE_COMPUTE,
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_TRANSFER,
    QUEUE_TYPE_PRESENT,
    QUEUE_TYPE_COUNT,
};

class VulkanDevice : public GraphicsDevice {
public:
    void init(VkPhysicalDevice                 physicalDevice,
              VkPhysicalDeviceFeatures         features,
              const std::vector<const char *> &extension);

    void destroy() const;

public:
    VkResult createBuffer(BufferCreateInfo *pCreateInfo,
                          VulkanBuffer    **ppBuffer,
                          void             *data = nullptr);

    VkResult createImage(ImageCreateInfo *pCreateInfo,
                         VulkanImage    **ppImage);

    VkResult createImageView(ImageViewCreateInfo *pCreateInfo,
                             VulkanImageView    **ppImageView,
                             VulkanImage         *pImage);

    VkResult createFramebuffers(FramebufferCreateInfo *pCreateInfo,
                                VulkanFramebuffer    **ppFramebuffer,
                                uint32_t               attachmentCount,
                                VulkanImageView      **pAttachments);

    VkResult createRenderPass(RenderPassCreateInfo                       *createInfo,
                              VulkanRenderPass                          **ppRenderPass,
                              const std::vector<VkAttachmentDescription> &colorAttachments,
                              const VkAttachmentDescription              &depthAttachment);

    VkResult createSwapchain(VkSurfaceKHR surface, VulkanSwapChain **ppSwapchain, WindowData *data);

public:
    void destroyBuffer(VulkanBuffer *pBuffer);
    void destroyImage(VulkanImage *pImage);
    void destroyImageView(VulkanImageView *pImageView);
    void destroyFramebuffers(VulkanFramebuffer *pFramebuffer);
    void destoryRenderPass(VulkanRenderPass *pRenderpass);
    void destroySwapchain(VulkanSwapChain *pSwapchain);

public:
    void transitionImageLayout(VulkanImage  *image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);

    void copyBuffer(VulkanBuffer *srcBuffer,
                    VulkanBuffer *dstBuffer,
                    VkDeviceSize  size);

    void copyBufferToImage(VulkanBuffer *buffer,
                           VulkanImage  *image);

    void copyImage(VulkanImage *srcImage,
                   VulkanImage *dstImage);

public:
    void allocateCommandBuffers(VkCommandBuffer *cmdbuffer,
                                uint32_t         count);

    VkCommandPool createCommandPool(uint32_t                 queueFamilyIndex,
                                    VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;

    void immediateSubmit(QueueFlags flags, std::function<void(VkCommandBuffer cmd)> &&function) ;

    VkCommandBuffer beginSingleTimeCommands(QueueFlags flags = QUEUE_TYPE_GRAPHICS);

    void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                               QueueFlags         flags);

    void flushCommandBuffer(VkCommandBuffer commandBuffer,
                            VkQueue         queue,
                            VkCommandPool   pool,
                            bool            free = true) const;

    void flushCommandBuffer(VkCommandBuffer commandBuffer,
                            VkQueue         queue,
                            bool            free = true) const;

    void waitIdle();

public:
    VkCommandPool              &getCommandPoolWithQueue(QueueFlags type);
    VkPhysicalDevice            getPhysicalDevice() const;
    VkDevice                    getLogicalDevice() const;
    VkPhysicalDeviceFeatures   &getDeviceEnabledFeatures();
    VkPhysicalDeviceProperties &getDeviceProperties();
    uint32_t                   &GetQueueFamilyIndices(QueueFlags type);
    VkQueue                     getQueueByFlags(QueueFlags flags, uint32_t queueIndex = 0);

    VkFormat findDepthFormat() const;

private:
    bool     extensionSupported(std::string_view extension) const;
    VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    uint32_t findQueueFamilies(VkQueueFlags queueFlags) const;
    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

private:
    struct {
        VkDevice                             logicalDevice  = VK_NULL_HANDLE;
        VkPhysicalDevice                     physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties           properties;
        VkPhysicalDeviceFeatures             features;
        VkPhysicalDeviceFeatures             enabledFeatures;
        VkPhysicalDeviceMemoryProperties     memoryProperties;
        std::vector<std::string>             supportedExtensions;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    } _deviceInfo;

    struct {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
        uint32_t present;
    } _queueFamilyIndices;

    std::array<QueueFamily, QUEUE_TYPE_COUNT> _queues = {};

    VkCommandPool _drawCommandPool     = VK_NULL_HANDLE;
    VkCommandPool _transferCommandPool = VK_NULL_HANDLE;
    VkCommandPool _computeCommandPool  = VK_NULL_HANDLE;
};

} // namespace vkl

#endif // VKLDEVICE_H_
