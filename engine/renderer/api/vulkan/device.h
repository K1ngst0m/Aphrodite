#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "physicalDevice.h"
#include "renderer/device.h"
#include "renderer/gpuResource.h"
#include "vkInit.hpp"
#include "vkUtils.h"

namespace vkl
{
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
struct GraphicsPipelineCreateInfo;
struct ComputePipelineCreateInfo;
struct EffectInfo;

using QueueFamily = std::vector<VulkanQueue *>;
using QueueFamilyCommandPools = std::unordered_map<uint32_t, VulkanCommandPool *>;

enum DeviceCreateFlagBits
{

};
using DeviceCreateFlags = uint32_t;

struct DeviceCreateInfo
{
    DeviceCreateFlags flags;
    std::vector<const char *> enabledExtensions;
    VulkanPhysicalDevice *pPhysicalDevice = nullptr;
};

class VulkanDevice : public GraphicsDevice, public ResourceHandle<VkDevice>
{
private:
    VulkanDevice() = default;
public:
    static VkResult Create(const DeviceCreateInfo &createInfo, VulkanDevice **ppDevice);

    static void Destroy(VulkanDevice *pDevice);

public:
    VkResult createBuffer(const BufferCreateInfo &createInfo, VulkanBuffer **ppBuffer, void *data = nullptr);

    VkResult createImage(const ImageCreateInfo &createInfo, VulkanImage **ppImage);

    VkResult createImageView(const ImageViewCreateInfo &createInfo, VulkanImageView **ppImageView, VulkanImage *pImage);

    VkResult createFramebuffers(FramebufferCreateInfo *pCreateInfo, VulkanFramebuffer **ppFramebuffer,
                                uint32_t attachmentCount, VulkanImageView **pAttachments);

    VkResult createRenderPass(RenderPassCreateInfo *createInfo, VulkanRenderPass **ppRenderPass,
                              const std::vector<VkAttachmentDescription> &colorAttachments);

    VkResult createRenderPass(RenderPassCreateInfo *createInfo, VulkanRenderPass **ppRenderPass,
                              const std::vector<VkAttachmentDescription> &colorAttachments,
                              const VkAttachmentDescription &depthAttachment);

    VkResult createRenderPass(RenderPassCreateInfo *createInfo, VulkanRenderPass **ppRenderPass,
                              const VkAttachmentDescription &depthAttachment);

    VkResult createSwapchain(VkSurfaceKHR surface, VulkanSwapChain **ppSwapchain, void *windowHandle);

    VkResult createCommandPool(VulkanCommandPool **ppPool, uint32_t queueFamilyIndex,
                               VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkResult createGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo, VulkanRenderPass *pRenderPass,
                                    VulkanPipeline **ppPipeline);

    VkResult createComputePipeline(const ComputePipelineCreateInfo &createInfo, VulkanPipeline **ppPipeline);

    VkResult createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                       VulkanDescriptorSetLayout **ppDescriptorSetLayout);

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
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers, VulkanQueue *pQueue);

    void freeCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers);

    VulkanCommandBuffer *beginSingleTimeCommands(VulkanQueue * pQueue);

    void endSingleTimeCommands(VulkanCommandBuffer *commandBuffer);

    void waitIdle();

public:
    VulkanSyncPrimitivesPool *getSyncPrimitiviesPool() { return m_syncPrimitivesPool; }
    VulkanShaderCache *getShaderCache() { return m_shaderCache; }
    VulkanCommandPool *getCommandPoolWithQueue(VulkanQueue *queue);
    VulkanPhysicalDevice *getPhysicalDevice() const;
    VulkanQueue *getQueueByFlags(QueueTypeFlags flags, uint32_t queueIndex = 0);
    VkFormat getDepthFormat() const;

private:
    VulkanPhysicalDevice *m_physicalDevice;
    VkPhysicalDeviceFeatures m_enabledFeatures;

    std::vector<QueueFamily> m_queues = {};

    QueueFamilyCommandPools m_commandPools;

    VulkanSyncPrimitivesPool *m_syncPrimitivesPool = nullptr;
    VulkanShaderCache *m_shaderCache = nullptr;

    DeviceCreateInfo m_createInfo;
};

}  // namespace vkl

#endif  // VKLDEVICE_H_
