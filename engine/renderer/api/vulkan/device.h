#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "physicalDevice.h"
#include "renderer/device.h"
#include "renderer/gpuResource.h"
#include "vkInit.hpp"
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
class PipelineBuilder;
class ShaderEffect;
class ShaderPass;
class VulkanPipeline;
class VulkanQueue;
class VulkanSyncPrimitivesPool;
struct RenderPassCreateInfo;
struct PipelineCreateInfo;
struct EffectInfo;

using QueueFamily             = std::vector<VulkanQueue *>;
using QueueFamilyCommandPools = std::unordered_map<uint32_t, VulkanCommandPool *>;

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

    VkResult createGraphicsPipeline(const PipelineCreateInfo *pCreateInfo,
                                    EffectInfo               *pEffectInfo,
                                    VulkanRenderPass         *pRenderPass,
                                    VulkanPipeline          **ppPipeline);

    VkResult createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                       VulkanDescriptorSetLayout      **ppDescriptorSetLayout);

public:
    void destroyBuffer(VulkanBuffer *pBuffer);
    void destroyImage(VulkanImage *pImage);
    void destroyImageView(VulkanImageView *pImageView);
    void destroyFramebuffers(VulkanFramebuffer *pFramebuffer);
    void destoryRenderPass(VulkanRenderPass *pRenderpass);
    void destroySwapchain(VulkanSwapChain *pSwapchain);
    void destroyCommandPool(VulkanCommandPool *pPool);
    void destroyPipeline(VulkanPipeline *pipeline);
    void destroyDescriptorSetLayout(VulkanDescriptorSetLayout *pLayout);

public:
    VkResult allocateCommandBuffers(uint32_t              commandBufferCount,
                                    VulkanCommandBuffer **ppCommandBuffers,
                                    VkQueueFlags          flags = VK_QUEUE_GRAPHICS_BIT);

    void freeCommandBuffers(uint32_t              commandBufferCount,
                            VulkanCommandBuffer **ppCommandBuffers);

    VulkanCommandBuffer *beginSingleTimeCommands(VkQueueFlags flags = VK_QUEUE_GRAPHICS_BIT);

    void endSingleTimeCommands(VulkanCommandBuffer *commandBuffer);

    void waitIdle();

public:
    VulkanCommandPool        *getCommandPoolWithQueue(VulkanQueue *queue);
    VulkanPhysicalDevice     *getPhysicalDevice() const;
    VulkanQueue              *getQueueByFlags(VkQueueFlags flags, uint32_t queueIndex = 0);
    VkFormat                  getDepthFormat() const;
    VulkanSyncPrimitivesPool *getSyncPrimitiviesPool();
    VulkanShaderCache        *getShaderCache();

private:
    VulkanPhysicalDevice    *_physicalDevice;
    VkPhysicalDeviceFeatures _enabledFeatures;

    std::vector<QueueFamily> _queues = {};

    QueueFamilyCommandPools _commandPools;

    VulkanSyncPrimitivesPool *_syncPrimitivesPool = nullptr;
    VulkanShaderCache        *_shaderCache        = nullptr;

    DeviceCreateInfo _createInfo;
};

} // namespace vkl

#endif // VKLDEVICE_H_
