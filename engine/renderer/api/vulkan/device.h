#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "physicalDevice.h"
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
class VulkanCommandBuffer;
class VulkanCommandPool;
class VulkanShaderModule;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;
class VulkanShaderCache;
class WindowData;
struct RenderPassCreateInfo;

using QueueFamily = std::vector<VkQueue>;

struct DeviceCreateInfo {
    const void        *pNext = nullptr;
    uint32_t           enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    uint32_t           enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
    VkQueueFlags       requestQueueTypes = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
};

class VulkanDevice : public GraphicsDevice, public ResourceHandle<VkDevice> {
public:
    static VkResult Create(VulkanPhysicalDevice   *pPhysicalDevice,
                           const DeviceCreateInfo *pCreateInfo,
                           VulkanDevice          **ppDevice);

    static void Destroy(VulkanDevice *pDevice);

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

    VkResult createSwapchain(VkSurfaceKHR      surface,
                             VulkanSwapChain **ppSwapchain,
                             WindowData       *data);

    VkResult createCommandPool(VulkanCommandPool      **ppPool,
                               uint32_t                 queueFamilyIndex,
                               VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

public:
    void destroyBuffer(VulkanBuffer *pBuffer);
    void destroyImage(VulkanImage *pImage);
    void destroyImageView(VulkanImageView *pImageView);
    void destroyFramebuffers(VulkanFramebuffer *pFramebuffer);
    void destoryRenderPass(VulkanRenderPass *pRenderpass);
    void destroySwapchain(VulkanSwapChain *pSwapchain);
    void destroyCommandPool(VulkanCommandPool *pPool);

public:
    VkResult allocateCommandBuffers(uint32_t              commandBufferCount,
                                    VulkanCommandBuffer **ppCommandBuffers,
                                    QueueFlags            flags = QUEUE_TYPE_GRAPHICS);

    void freeCommandBuffers(uint32_t              commandBufferCount,
                            VulkanCommandBuffer **ppCommandBuffers);

    VulkanCommandBuffer *beginSingleTimeCommands(QueueFlags flags = QUEUE_TYPE_GRAPHICS);

    void endSingleTimeCommands(VulkanCommandBuffer *commandBuffer,
                               QueueFlags           flags = QUEUE_TYPE_GRAPHICS);

    void waitIdle();

public:
    VulkanCommandPool    *getCommandPoolWithQueue(QueueFlags type);
    VulkanPhysicalDevice *getPhysicalDevice() const;
    VkQueue               getQueueByFlags(QueueFlags flags, uint32_t queueIndex = 0);
    VkFormat              getDepthFormat() const;

private:
    VulkanPhysicalDevice    *_physicalDevice;
    VkPhysicalDeviceFeatures _enabledFeatures;

    std::array<QueueFamily, QUEUE_TYPE_COUNT> _queues = {};

    VulkanCommandPool *_drawCommandPool     = nullptr;
    VulkanCommandPool *_transferCommandPool = nullptr;
    VulkanCommandPool *_computeCommandPool  = nullptr;

    DeviceCreateInfo _createInfo;
};

} // namespace vkl

#endif // VKLDEVICE_H_
