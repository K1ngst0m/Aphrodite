#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "buffer.h"
#include "commandBuffer.h"
#include "descriptorPool.h"
#include "descriptorSetLayout.h"
#include "image.h"
#include "sampler.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"
#include "vkInit.h"
#include "vkUtils.h"

namespace aph::vk
{

enum DeviceCreateFlagBits
{
    // TODO
};
using DeviceCreateFlags = uint32_t;

struct DeviceCreateInfo
{
    DeviceCreateFlags        flags;
    std::vector<const char*> enabledExtensions;
    PhysicalDevice*          pPhysicalDevice = nullptr;
};

class Device : public ResourceHandle<VkDevice, DeviceCreateInfo>
{
private:
    Device(const DeviceCreateInfo& createInfo, PhysicalDevice* pPhysicalDevice, VkDevice handle);

public:
    static VkResult Create(const DeviceCreateInfo& createInfo, Device** ppDevice);
    static void     Destroy(Device* pDevice);

public:
    VkResult createCubeMap(const std::array<std::shared_ptr<ImageInfo>, 6>& images, Image** ppImage);
    VkResult createDeviceLocalBuffer(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const void* data);
    VkResult createDeviceLocalImage(const ImageCreateInfo& createInfo, Image** ppImage,
                                    const std::vector<uint8_t>& data);
    VkResult executeSingleCommands(QueueType type, const std::function<void(CommandBuffer* pCmdBuffer)>&& func);
    VkResult executeSingleCommands(Queue* queue, const std::function<void(CommandBuffer* pCmdBuffer)>&& func);

public:
    VkResult createSampler(const VkSamplerCreateInfo& createInfo, Sampler** ppSampler, bool immutable);
    VkResult createBuffer(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const void* data = nullptr,
                          bool persistmentMap = false);
    VkResult createImage(const ImageCreateInfo& createInfo, Image** ppImage);
    VkResult createImageView(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, Image* pImage);
    VkResult createShaderProgram(ShaderProgram** ppProgram, Shader* vs, Shader* fs,
                                 const ImmutableSamplerBank* samplerBank = nullptr);
    VkResult createShaderProgram(ShaderProgram** ppProgram, Shader* cs,
                                 const ImmutableSamplerBank* samplerBank = nullptr);
    VkResult createSwapchain(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain);
    VkResult createCommandPool(const CommandPoolCreateInfo& createInfo, VkCommandPool* ppPool);
    VkResult createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, Pipeline** ppPipeline);
    VkResult createComputePipeline(const ComputePipelineCreateInfo& createInfo, Pipeline** ppPipeline);
    VkResult createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo,
                                       DescriptorSetLayout**                  ppDescriptorSetLayout);
    VkResult createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindingds,
                                       DescriptorSetLayout**                            ppDescriptorSetLayout);

    VkResult resetCommandPool(VkCommandPool pPool);

public:
    void destroyShaderProgram(ShaderProgram* pProgram);
    void destroyBuffer(Buffer* pBuffer);
    void destroyImage(Image* pImage);
    void destroyImageView(ImageView* pImageView);
    void destroySwapchain(SwapChain* pSwapchain);
    void destroyCommandPool(VkCommandPool pPool);
    void destroyPipeline(Pipeline* pipeline);
    void destroyDescriptorSetLayout(DescriptorSetLayout* pLayout);
    void destroySampler(Sampler* pSampler);

public:
    VkResult flushMemory(VkDeviceMemory memory, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    VkResult invalidateMemory(VkDeviceMemory memory, VkDeviceSize size = 0, VkDeviceSize offset = VK_WHOLE_SIZE);
    VkResult bindMemory(Buffer* pBuffer, VkDeviceSize offset = 0);
    VkResult bindMemory(Image* pImage, VkDeviceSize offset = 0);
    VkResult mapMemory(Buffer* pBuffer, void* mapped = nullptr, VkDeviceSize offset = 0,
                       VkDeviceSize size = VK_WHOLE_SIZE);
    void     unMapMemory(Buffer* pBuffer);

public:
    VolkDeviceTable* getDeviceTable() { return &m_table; }
    VkCommandPool    getCommandPoolWithQueue(Queue* queue);
    Queue*           getQueueByFlags(QueueType flags, uint32_t queueIndex = 0);
    VkResult allocateThreadCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, Queue* pQueue);
    VkResult allocateCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, Queue* pQueue);
    void     freeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers);

    VkResult waitIdle();
    VkResult waitForFence(const std::vector<VkFence>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);
    PhysicalDevice*          getPhysicalDevice() const;
    VkFormat                 getDepthFormat() const;
    VkPhysicalDeviceFeatures getFeatures() const { return m_supportedFeatures; }

private:
    VkPhysicalDeviceFeatures                    m_supportedFeatures{};
    PhysicalDevice*                             m_physicalDevice{};
    VolkDeviceTable                             m_table{};
    std::vector<QueueFamily>                    m_queues;
    std::unordered_map<uint32_t, VkCommandPool> m_commandPools;
    std::vector<VkCommandPool>                  m_threadCommandPools;
};

}  // namespace aph::vk

#endif  // VKLDEVICE_H_
