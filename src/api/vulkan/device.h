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

    static auto resourceTypeToString(ResourceType type) -> const char*;
    auto trackCreation(ResourceType type) -> void;
    auto trackDestruction(ResourceType type) -> void;
    auto generateReport() const -> std::string;

    auto getCreatedCount(ResourceType type) const -> uint32_t;
    auto getDestroyedCount(ResourceType type) const -> uint32_t;
    auto getActiveCount(ResourceType type) const -> uint32_t;

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
    auto initialize(const DeviceCreateInfo& createInfo) -> Result;

    // Make SamplerPool a friend class
    friend class SamplerPool;

public:
    Device(const Device&)                    = delete;
    Device(Device&&)                         = delete;
    auto operator=(const Device&) -> Device& = delete;
    auto operator=(Device&&) -> Device&      = delete;

    // Factory methods
    static auto Destroy(Device* pDevice) -> void;
    static auto Create(const DeviceCreateInfo& createInfo) -> Expected<Device*>;

public:
    template <typename TCreateInfo,
              typename TResource  = typename ResourceTraits<std::decay_t<TCreateInfo>>::ResourceType,
              typename TDebugName = std::string>
    auto create(TCreateInfo&& createInfo, TDebugName&& debugName = {},
                const std::source_location& location = std::source_location::current()) -> Expected<TResource*>;

    template <typename TResource>
    auto destroy(TResource* pResource, const std::source_location& location = std::source_location::current()) -> void;

public:
    auto getDeviceAddress(Buffer* pBuffer) const -> DeviceAddress;
    auto getBindlessResource() const -> BindlessResource*;
    auto getCommandBufferAllocator() const -> CommandBufferAllocator*;
    auto waitIdle() -> Result;
    auto waitForFence(ArrayProxy<Fence*> fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) -> Result;

    auto acquireSemaphore() -> Semaphore*;
    auto acquireFence(bool isSignaled) -> Fence*;
    auto releaseSemaphore(Semaphore* semaphore) -> Result;
    auto releaseFence(Fence* pFence) -> Result;

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    auto executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems = {},
                        ArrayProxy<Semaphore*> signalSems = {}, Fence* pFence = nullptr) -> void;

public:
    auto flushMemory(Image* pImage, Range range = {}) const -> Result;
    auto flushMemory(Buffer* pBuffer, Range range = {}) const -> Result;
    auto invalidateMemory(Image* pImage, Range range = {}) const -> Result;
    auto invalidateMemory(Buffer* pBuffer, Range range = {}) const -> Result;

    auto mapMemory(Buffer* pBuffer) const -> void*;
    auto unMapMemory(Buffer* pBuffer) const -> void;

public:
    auto getPhysicalDevice() const -> PhysicalDevice*;
    auto getEnabledFeatures() const -> GPUFeature;
    auto getDepthFormat() const -> Format;
    auto getQueue(QueueType type, uint32_t queueIndex = 0) -> Queue*;

    auto getTimeQueryResults(::vk::QueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                             TimeUnit unitType = TimeUnit::Seconds) -> double;

public:
    auto determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType) -> ::vk::PipelineStageFlags;

    template <ResourceHandleType TObject>
    auto setDebugObjectName(TObject* object, auto&& name) -> Result;

    template <typename TObject>
        requires(!ResourceHandleType<TObject>)
    auto setDebugObjectName(TObject object, std::string_view name) -> Result;

    // Resource statistics methods
    auto getResourceStatsReport() const -> std::string;
    auto getResourceStats() const -> const ResourceStats&;
    auto getSamplerPool() const -> SamplerPool*;
    auto getSampler(PresetSamplerType type) const -> Sampler*;

private:
    auto createImpl(const SamplerCreateInfo& createInfo, bool isPoolInitialization = false) -> Expected<Sampler*>;
    auto createImpl(const BufferCreateInfo& createInfo) -> Expected<Buffer*>;
    auto createImpl(const ImageCreateInfo& createInfo) -> Expected<Image*>;
    auto createImpl(const ImageViewCreateInfo& createInfo) -> Expected<ImageView*>;
    auto createImpl(const SwapChainCreateInfo& createInfo) -> Expected<SwapChain*>;
    auto createImpl(const ProgramCreateInfo& createInfo) -> Expected<ShaderProgram*>;
    auto createImpl(const DescriptorSetLayoutCreateInfo& createInfo) -> Expected<DescriptorSetLayout*>;
    auto createImpl(const PipelineLayoutCreateInfo& createInfo) -> Expected<PipelineLayout*>;

    auto destroyImpl(Buffer* pBuffer) -> void;
    auto destroyImpl(Image* pImage) -> void;
    auto destroyImpl(ImageView* pImageView) -> void;
    auto destroyImpl(SwapChain* pSwapchain) -> void;
    auto destroyImpl(Sampler* pSampler) -> void;
    auto destroyImpl(ShaderProgram* pProgram) -> void;
    auto destroyImpl(DescriptorSetLayout* pSetLayout) -> void;
    auto destroyImpl(PipelineLayout* pLayout) -> void;

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
    auto trackResourceCreation() -> void;

    template <typename TResource>
    auto trackResourceDestruction() -> void;
};

// Include template implementations
#include "deviceImpl.inl"

} // namespace aph::vk
