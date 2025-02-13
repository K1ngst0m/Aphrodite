#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "instance.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "descriptorSet.h"
#include "image.h"
#include "sampler.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"

namespace aph::vk
{
class DeviceAllocator;

enum class TimeUnit
{
    Seconds,
    MillSeconds,
    MicroSeconds,
    NanoSeconds
};

enum DeviceCreateFlagBits
{
    // TODO
};
using DeviceCreateFlags = uint32_t;

struct DeviceCreateInfo
{
    DeviceCreateFlags        flags;
    GPUFeature               enabledFeatures = {};
    PhysicalDevice*          pPhysicalDevice = nullptr;
    Instance*                pInstance       = nullptr;
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
    Result create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, std::string_view debugName = "");

    void destroy(Buffer* pBuffer);
    void destroy(Image* pImage);
    void destroy(ImageView* pImageView);
    void destroy(SwapChain* pSwapchain);
    void destroy(Sampler* pSampler);
    void destroy(ShaderProgram* pProgram);

public:
    Pipeline*    acquirePipeline(const GraphicsPipelineCreateInfo& createInfo, std::string_view debugName = "");
    Pipeline*    acquirePipeline(const ComputePipelineCreateInfo& createInfo, std::string_view debugName = "");
    CommandPool* acquireCommandPool(const CommandPoolCreateInfo& info);
    Semaphore*   acquireSemaphore();
    Fence*       acquireFence(bool isSignaled);
    Result       releaseSemaphore(Semaphore* semaphore);
    Result       releaseFence(Fence* pFence);
    Result       releaseCommandPool(CommandPool* pPool);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func,
                               const std::vector<Semaphore*>& waitSems   = {},
                               const std::vector<Semaphore*>& signalSems = {}, Fence* pFence = nullptr);

public:
    Result flushMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result invalidateMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result mapMemory(Buffer* pBuffer, void** ppMapped) const;
    void   unMapMemory(Buffer* pBuffer) const;

public:
    VolkDeviceTable* getDeviceTable() { return &m_table; }
    PhysicalDevice*  getPhysicalDevice() const { return m_physicalDevice; }
    VkFormat         getDepthFormat() const;
    Queue*           getQueue(QueueType type, uint32_t queueIndex = 0);

    double getTimeQueryResults(VkQueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                               TimeUnit unitType = TimeUnit::Seconds);

public:
    VkPipelineStageFlags determinePipelineStageFlags(VkAccessFlags accessFlags, QueueType queueType);

public:
    void   waitIdle();
    Result waitForFence(const std::vector<Fence*>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

private:
    VkPhysicalDeviceFeatures m_supportedFeatures{};
    PhysicalDevice*          m_physicalDevice{};
    VolkDeviceTable          m_table{};

    HashMap<QueueType, SmallVector<Queue*>> m_queues;

private:
    struct ResourceObjectPool
    {
        DeviceAllocator*                    gpu;
        ThreadSafeObjectPool<Buffer>        buffer;
        ThreadSafeObjectPool<Image>         image;
        ThreadSafeObjectPool<Sampler>       sampler;
        ThreadSafeObjectPool<ImageView>     imageView;
        PipelineAllocator                   pipeline;
        ThreadSafeObjectPool<ShaderProgram> program;
        ThreadSafeObjectPool<Queue>         queue;
        CommandPoolAllocator                commandPool;
        SyncPrimitiveAllocator              syncPrimitive;

        ResourceObjectPool(Device* pDevice) : pipeline(pDevice), commandPool(pDevice), syncPrimitive(pDevice) {}
    } m_resourcePool;
};

}  // namespace aph::vk

#endif  // VKLDEVICE_H_
