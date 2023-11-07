#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

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
#include "vkInit.h"
#include "vkUtils.h"

namespace aph::vk
{

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
    Result create(const ProgramCreateInfo& createInfo, ShaderProgram** ppPipeline, std::string_view debugName = "");
    Result create(const GraphicsPipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName = "");
    Result create(const ComputePipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName = "");

    void destroy(Buffer* pBuffer);
    void destroy(Image* pImage);
    void destroy(ImageView* pImageView);
    void destroy(SwapChain* pSwapchain);
    void destroy(Pipeline* pipeline);
    void destroy(Sampler* pSampler);
    void destroy(ShaderProgram* pProgram);

    // TODO explicit "destroyable" object
    template <typename... Args>
    void destroy(Args... args)
    {
        auto destructor = [this](auto* ptr) {
            APH_ASSERT(ptr != nullptr);
            using Type = std::remove_pointer_t<decltype(ptr)>;
            static_assert(Type::value, "Unsupported type for destroy");
            destroy(ptr);
        };
        (destructor(args), ...);
    }

public:
    CommandPool* acquireCommandPool(const CommandPoolCreateInfo& info);
    Semaphore*   acquireSemaphore();
    Fence*       acquireFence(bool isSignaled = true);
    Result       releaseSemaphore(Semaphore* semaphore);
    Result       releaseFence(Fence* pFence);
    Result       releaseCommandPool(CommandPool* pPool);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func);

public:
    Result flushMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result invalidateMemory(VkDeviceMemory memory, MemoryRange range = {});
    Result mapMemory(Buffer* pBuffer, void* mapped = nullptr, MemoryRange range = {});
    void   unMapMemory(Buffer* pBuffer);

public:
    VolkDeviceTable* getDeviceTable() { return &m_table; }
    PhysicalDevice*  getPhysicalDevice() const { return m_physicalDevice; }
    VkFormat         getDepthFormat() const;
    Queue*           getQueue(QueueType flags, uint32_t queueIndex = 0);

    double getTimeQueryResults(VkQueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                               TimeUnit unitType = TimeUnit::Seconds);

public:
    void   waitIdle();
    Result waitForFence(const std::vector<Fence*>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

private:
    VkPhysicalDeviceFeatures m_supportedFeatures{};
    PhysicalDevice*          m_physicalDevice{};
    VolkDeviceTable          m_table{};

    using QueueFamily = std::vector<Queue*>;
    std::vector<QueueFamily> m_queues;

private:
    struct ResourceObjectPool
    {
        ThreadSafeObjectPool<Buffer>        buffer;
        ThreadSafeObjectPool<Image>         image;
        ThreadSafeObjectPool<Sampler>       sampler;
        ThreadSafeObjectPool<ImageView>     imageView;
        ThreadSafeObjectPool<Pipeline>      pipeline;
        ThreadSafeObjectPool<ShaderProgram> program;
        ThreadSafeObjectPool<Queue>         queue;
        CommandPoolAllocator                commandPool;
        SyncPrimitiveAllocator              syncPrimitive;

        ResourceObjectPool(Device* pDevice) : commandPool(pDevice), syncPrimitive(pDevice) {}
    } m_resourcePool;
};

}  // namespace aph::vk

#endif  // VKLDEVICE_H_
