#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "descriptorPool.h"
#include "descriptorSetLayout.h"
#include "framebuffer.h"
#include "image.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "renderpass.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"
#include "vkInit.hpp"
#include "vkUtils.h"

namespace vkl
{

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

class VulkanDevice : public ResourceHandle<VkDevice, DeviceCreateInfo>
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
    VkResult createFramebuffers(const FramebufferCreateInfo &createInfo, VulkanFramebuffer **ppFramebuffer);
    VkResult createRenderPass(const RenderPassCreateInfo &createInfo, VulkanRenderPass **ppRenderPass);
    VkResult createSwapchain(const SwapChainCreateInfo &createInfo, VulkanSwapChain **ppSwapchain);
    VkResult createCommandPool(const CommandPoolCreateInfo&createInfo, VulkanCommandPool **ppPool);
    VkResult createGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo, VulkanRenderPass *pRenderPass, VulkanPipeline **ppPipeline);
    VkResult createComputePipeline(const ComputePipelineCreateInfo &createInfo, VulkanPipeline **ppPipeline);
    VkResult createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo *pCreateInfo, VulkanDescriptorSetLayout **ppDescriptorSetLayout);

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
    VulkanCommandPool *getCommandPoolWithQueue(VulkanQueue *queue);
    VulkanQueue *getQueueByFlags(QueueTypeFlags flags, uint32_t queueIndex = 0);
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers,
                                    VulkanQueue *pQueue);
    void freeCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers);
    VkResult executeSingleCommands(QueueTypeFlags type,
                                   const std::function<void(VulkanCommandBuffer *pCmdBuffer)> &&func);

    VkResult waitIdle();
    VkResult waitForFence(const std::vector<VkFence> &fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);
    VulkanPhysicalDevice *getPhysicalDevice() const;
    VkFormat getDepthFormat() const;

private:
    VulkanPhysicalDevice *m_physicalDevice;
    std::vector<QueueFamily> m_queues;
    QueueFamilyCommandPools m_commandPools;
};

}  // namespace vkl

#endif  // VKLDEVICE_H_
