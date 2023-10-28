#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "buffer.h"
#include "commandBuffer.h"
#include "descriptorSet.h"
#include "image.h"
#include "sampler.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"
#include "vkInit.h"
#include "vkUtils.h"
#include "common/objectPool.h"

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
    Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle);

public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo& createInfo);
    static void                    Destroy(Device* pDevice);

public:
    Result create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, std::string_view debugName = "");
    Result create(const BufferCreateInfo& createInfo, Buffer** ppBuffer, std::string_view debugName = "");
    Result create(const ImageCreateInfo& createInfo, Image** ppImage, std::string_view debugName = "");
    Result create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, std::string_view debugName = "");
    Result create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, std::string_view debugName = "");
    Result create(const GraphicsPipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName = "");
    Result create(const ComputePipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName = "");

    void destroy(Buffer* pBuffer);
    void destroy(Image* pImage);
    void destroy(ImageView* pImageView);
    void destroy(SwapChain* pSwapchain);
    void destroy(Pipeline* pipeline);
    void destroy(Sampler* pSampler);

    template <typename... Args>
    void destroy(Args... args)
    {
        auto destructor = [this](auto* ptr) {
            APH_ASSERT(ptr != nullptr);
            using Type = std::remove_pointer_t<decltype(ptr)>;
            static_assert(Type::value, "Unsupported type for destroy");
            destroy(ptr);
        };
        (destructor(args), ...);  // Use fold expression to call the lambda for each argument
    }

public:
    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;

    void   executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func);
    Result allocateCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, Queue* pQueue);
    void   freeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers);

public:
    Result flushMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result invalidateMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result mapMemory(Buffer* pBuffer, void* mapped = nullptr, MemoryRange range = {});
    void   unMapMemory(Buffer* pBuffer);

public:
    Semaphore* acquireSemaphore()
    {
        Semaphore* semaphore;
        m_resourcePool.syncPrimitive.acquireSemaphore(1, &semaphore);
        return semaphore;
    }

    Result releaseSemaphore(Semaphore* semaphore)
    {
        if(semaphore != VK_NULL_HANDLE)
        {
            auto result = m_resourcePool.syncPrimitive.ReleaseSemaphores(1, &semaphore);
            if(result != VK_SUCCESS)
            {
                return Result::RuntimeError;
            }
        }
        return Result::Success;
    }

    Fence* acquireFence(bool isSignaled = true)
    {
        Fence* pFence = {};
        m_resourcePool.syncPrimitive.acquireFence(&pFence);
        return pFence;
    }

    Result releaseFence(Fence* pFence)
    {
        auto res = m_resourcePool.syncPrimitive.releaseFence(pFence);
        if(res != VK_SUCCESS)
        {
            return Result::RuntimeError;
        }
        return Result::Success;
    }

public:
    VolkDeviceTable*                getDeviceTable() { return &m_table; }
    PhysicalDevice*                 getPhysicalDevice() const { return m_physicalDevice; }
    const VkPhysicalDeviceFeatures& getFeatures() const { return m_supportedFeatures; }
    VkFormat                        getDepthFormat() const;
    Queue*                          getQueueByFlags(QueueType flags, uint32_t queueIndex = 0);

public:
    void   waitIdle();
    Result waitForFence(const std::vector<VkFence>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

private:
    VkCommandPool getCommandPoolWithQueue(Queue* queue);

private:
    VkPhysicalDeviceFeatures                    m_supportedFeatures{};
    PhysicalDevice*                             m_physicalDevice{};
    VolkDeviceTable                             m_table{};
    std::vector<QueueFamily>                    m_queues;
    std::unordered_map<uint32_t, VkCommandPool> m_commandPools;

private:
    struct ResourceObjectPool
    {
        ThreadSafeObjectPool<Buffer>        buffer;
        ThreadSafeObjectPool<Image>         image;
        ThreadSafeObjectPool<Sampler>       sampler;
        ThreadSafeObjectPool<ImageView>     imageView;
        ThreadSafeObjectPool<Pipeline>      pipeline;
        ThreadSafeObjectPool<ShaderProgram> program;
        SyncPrimitivesPool                  syncPrimitive;

        ResourceObjectPool(Device* pDevcie): syncPrimitive(pDevcie){}
    } m_resourcePool;
};

}  // namespace aph::vk

#endif  // VKLDEVICE_H_
