#pragma once

#include "bindless.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandBufferAllocator.h"
#include "common/timer.h"
#include "descriptorSet.h"
#include "forward.h"
#include "image.h"
#include "instance.h"
#include "physicalDevice.h"
#include "queue.h"
#include "sampler.h"
#include "samplerPool.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"

#include "api/deviceAllocator.h"

namespace aph::vk
{

// Resource statistics tracking
class ResourceStats
{
public:
    enum class ResourceType : uint8_t
    {
        eBuffer,
        eImage,
        eImageView,
        eSampler,
        eShaderProgram,
        eDescriptorSetLayout,
        ePipelineLayout,
        eSwapChain,
        eCommandBuffer,
        eQueue,
        eFence,
        eSemaphore,
        eCount
    };

    static const char* resourceTypeToString(ResourceType type);
    void trackCreation(ResourceType type);
    void trackDestruction(ResourceType type);
    std::string generateReport() const;

    uint32_t getCreatedCount(ResourceType type) const;
    uint32_t getDestroyedCount(ResourceType type) const;
    uint32_t getActiveCount(ResourceType type) const;

private:
    mutable std::mutex m_mutex;
    std::array<std::atomic<uint32_t>, static_cast<size_t>(ResourceType::eCount)> m_created{};
    std::array<std::atomic<uint32_t>, static_cast<size_t>(ResourceType::eCount)> m_destroyed{};
    std::array<std::atomic<uint32_t>, static_cast<size_t>(ResourceType::eCount)> m_active{};
};

struct DeviceCreateInfo
{
    GPUFeature enabledFeatures      = {};
    PhysicalDevice* pPhysicalDevice = nullptr;
    Instance* pInstance             = nullptr;
};

// Type traits to map CreateInfo types to Resource types
template <typename TCreateInfo>
struct ResourceTraits;

class Device : public ResourceHandle<::vk::Device, DeviceCreateInfo>
{
private:
    Device(const CreateInfoType& createInfo, HandleType handle);
    ~Device() = default;
    Result initialize(const DeviceCreateInfo& createInfo);

    // Make SamplerPool a friend class
    friend class SamplerPool;

public:
    Device(const Device&)            = delete;
    Device(Device&&)                 = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&)      = delete;

    // Factory methods
    static Expected<Device*> Create(const DeviceCreateInfo& createInfo);
    static void Destroy(Device* pDevice);

public:
    template <typename TCreateInfo,
              typename TResource  = typename ResourceTraits<std::decay_t<TCreateInfo>>::ResourceType,
              typename TDebugName = std::string>
    Expected<TResource*> create(TCreateInfo&& createInfo, TDebugName&& debugName = {},
                                const std::source_location& location = std::source_location::current());

    template <typename TResource>
    void destroy(TResource* pResource, const std::source_location& location = std::source_location::current());

public:
    DeviceAddress getDeviceAddress(Buffer* pBuffer) const;
    BindlessResource* getBindlessResource() const;
    CommandBufferAllocator* getCommandBufferAllocator() const
    {
        return m_resourcePool.commandBufferAllocator.get();
    }
    Result waitIdle();
    Result waitForFence(ArrayProxy<Fence*> fences, bool waitAll = true, uint64_t timeout = UINT64_MAX);

    Semaphore* acquireSemaphore();
    Fence* acquireFence(bool isSignaled);
    Result releaseSemaphore(Semaphore* semaphore);
    Result releaseFence(Fence* pFence);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems = {},
                        ArrayProxy<Semaphore*> signalSems = {}, Fence* pFence = nullptr);

public:
    Result flushMemory(Image* pImage, Range range = {}) const;
    Result flushMemory(Buffer* pBuffer, Range range = {}) const;
    Result invalidateMemory(Image* pImage, Range range = {}) const;
    Result invalidateMemory(Buffer* pBuffer, Range range = {}) const;

    void* mapMemory(Buffer* pBuffer) const;
    void unMapMemory(Buffer* pBuffer) const;

public:
    PhysicalDevice* getPhysicalDevice() const;
    GPUFeature getEnabledFeatures() const;
    Format getDepthFormat() const;
    Queue* getQueue(QueueType type, uint32_t queueIndex = 0);

    double getTimeQueryResults(::vk::QueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                               TimeUnit unitType = TimeUnit::Seconds);

public:
    ::vk::PipelineStageFlags determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType);

    template <ResourceHandleType TObject>
    Result setDebugObjectName(TObject* object, auto&& name);

    template <typename TObject>
        requires(!ResourceHandleType<TObject>)
    Result setDebugObjectName(TObject object, std::string_view name);

    // Resource statistics methods
    std::string getResourceStatsReport() const;
    const ResourceStats& getResourceStats() const
    {
        return m_resourceStats;
    }

    SamplerPool* getSamplerPool() const
    {
        return m_resourcePool.samplerPool.get();
    }

    Sampler* getSampler(PresetSamplerType type) const
    {
        return m_resourcePool.samplerPool->getSampler(type);
    }

private:
    Expected<Sampler*> createImpl(const SamplerCreateInfo& createInfo, bool isPoolInitialization = false);
    Expected<Buffer*> createImpl(const BufferCreateInfo& createInfo);
    Expected<Image*> createImpl(const ImageCreateInfo& createInfo);
    Expected<ImageView*> createImpl(const ImageViewCreateInfo& createInfo);
    Expected<SwapChain*> createImpl(const SwapChainCreateInfo& createInfo);
    Expected<ShaderProgram*> createImpl(const ProgramCreateInfo& createInfo);
    Expected<DescriptorSetLayout*> createImpl(const DescriptorSetLayoutCreateInfo& createInfo);
    Expected<PipelineLayout*> createImpl(const PipelineLayoutCreateInfo& createInfo);

    void destroyImpl(Buffer* pBuffer);
    void destroyImpl(Image* pImage);
    void destroyImpl(ImageView* pImageView);
    void destroyImpl(SwapChain* pSwapchain);
    void destroyImpl(Sampler* pSampler);
    void destroyImpl(ShaderProgram* pProgram);
    void destroyImpl(DescriptorSetLayout* pSetLayout);
    void destroyImpl(PipelineLayout* pLayout);

private:
    HashMap<QueueType, SmallVector<Queue*>> m_queues;
    ResourceStats m_resourceStats;
    struct ResourcePool
    {
        std::unique_ptr<DeviceAllocator> deviceMemory;
        std::unique_ptr<CommandBufferAllocator> commandBufferAllocator;
        std::unique_ptr<SamplerPool> samplerPool;
        ThreadSafeObjectPool<Buffer> buffer;
        ThreadSafeObjectPool<Image> image;
        ThreadSafeObjectPool<PipelineLayout> pipelineLayout;
        ThreadSafeObjectPool<Sampler> sampler;
        ThreadSafeObjectPool<ImageView> imageView;
        ThreadSafeObjectPool<DescriptorSetLayout> setLayout;
        ThreadSafeObjectPool<ShaderProgram> program;
        ThreadSafeObjectPool<Queue> queue;
        SyncPrimitiveAllocator syncPrimitive;
        std::unique_ptr<BindlessResource> bindless;

        explicit ResourcePool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;

    // Helper methods for resource statistics
    template <typename TResource>
    void trackResourceCreation();

    template <typename TResource>
    void trackResourceDestruction();
};

// Include template implementations
#include "deviceImpl.inl"

} // namespace aph::vk
