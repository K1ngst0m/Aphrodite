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
#include "queryPool.h"
#include "queryPoolAllocator.h"
#include "resourceStats.h"
#include "sampler.h"
#include "samplerPool.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"

#include "api/deviceAllocator.h"

namespace aph::vk
{

struct DeviceCreateInfo
{
    GPUFeature enabledFeatures      = {};
    PhysicalDevice* pPhysicalDevice = nullptr;
    Instance* pInstance             = nullptr;
    bool enableResourceTracking = false;
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
    friend class SamplerPool;

public:
    Device(const Device&)                    = delete;
    Device(Device&&)                         = delete;
    auto operator=(const Device&) -> Device& = delete;
    auto operator=(Device&&) -> Device&      = delete;

    // Factory methods
    static auto Destroy(Device* pDevice) -> void;
    static auto Create(const DeviceCreateInfo& createInfo) -> Expected<Device*>;

    // Resource creation and destruction
    template <typename TCreateInfo,
              typename TResource  = typename ResourceTraits<std::decay_t<TCreateInfo>>::ResourceType,
              typename TDebugName = std::string>
    auto create(TCreateInfo&& createInfo, TDebugName&& debugName = {},
                const std::source_location& location = std::source_location::current()) -> Expected<TResource*>;

    template <typename TResource>
    auto destroy(TResource* pResource, const std::source_location& location = std::source_location::current()) -> void;

    // Resource management
    auto getDeviceAddress(Buffer* pBuffer) const -> DeviceAddress;
    auto getBindlessResource() const -> BindlessResource*;
    auto getCommandBufferAllocator() const -> CommandBufferAllocator*;
    auto getPhysicalDevice() const -> PhysicalDevice*;
    auto getEnabledFeatures() const -> GPUFeature;
    auto getDepthFormat() const -> Format;
    auto getQueue(QueueType type, uint32_t queueIndex = 0) -> Queue*;
    auto getSamplerPool() const -> SamplerPool*;
    auto getSampler(PresetSamplerType type) const -> Sampler*;

    // Memory operations
    auto flushMemory(Image* pImage, Range range = {}) const -> Result;
    auto flushMemory(Buffer* pBuffer, Range range = {}) const -> Result;
    auto invalidateMemory(Image* pImage, Range range = {}) const -> Result;
    auto invalidateMemory(Buffer* pBuffer, Range range = {}) const -> Result;
    auto mapMemory(Buffer* pBuffer) const -> void*;
    auto unMapMemory(Buffer* pBuffer) const -> void;

    // Synchronization
    auto waitIdle() -> Result;
    auto waitForFence(ArrayProxy<Fence*> fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) -> Result;
    auto acquireSemaphore() -> Semaphore*;
    auto acquireFence(bool isSignaled) -> Fence*;
    auto releaseSemaphore(Semaphore* semaphore) -> Result;
    auto releaseFence(Fence* pFence) -> Result;
    
    // Query pool management
    auto acquireQueryPool(QueryType type) -> QueryPool*;
    auto releaseQueryPool(QueryPool* pQueryPool) -> Result;
    auto resetQueryPools(QueryType type, CommandBuffer* pCommandBuffer) -> void;

    // Command execution
    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    auto executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems = {},
                        ArrayProxy<Semaphore*> signalSems = {}, Fence* pFence = nullptr) -> void;

    // Debug and profiling
    auto getTimeQueryResults(QueryPool* pQueryPool, uint32_t firstQuery, uint32_t secondQuery,
                             TimeUnit unitType = TimeUnit::Seconds) -> double;
    auto determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType) -> ::vk::PipelineStageFlags;
    template <ResourceHandleType TObject>
    auto setDebugObjectName(TObject* object, auto&& name) -> Result;
    template <typename TObject>
        requires(!ResourceHandleType<TObject>)
    auto setDebugObjectName(TObject object, std::string_view name) -> Result;

    // Resource statistics
    auto getResourceStatsReport() const -> std::string;
    auto getResourceStats() const -> const ResourceStats&;

private:
    auto createImpl(const SamplerCreateInfo& createInfo) -> Expected<Sampler*>;
    auto createImpl(const BufferCreateInfo& createInfo) -> Expected<Buffer*>;
    auto createImpl(const ImageCreateInfo& createInfo) -> Expected<Image*>;
    auto createImpl(const ImageViewCreateInfo& createInfo) -> Expected<ImageView*>;
    auto createImpl(const SwapChainCreateInfo& createInfo) -> Expected<SwapChain*>;
    auto createImpl(const ProgramCreateInfo& createInfo) -> Expected<ShaderProgram*>;
    auto createImpl(const DescriptorSetLayoutCreateInfo& createInfo) -> Expected<DescriptorSetLayout*>;
    auto createImpl(const PipelineLayoutCreateInfo& createInfo) -> Expected<PipelineLayout*>;
    auto createImpl(const QueryPoolCreateInfo& createInfo) -> Expected<QueryPool*>;

    auto destroyImpl(Buffer* pBuffer) -> void;
    auto destroyImpl(Image* pImage) -> void;
    auto destroyImpl(ImageView* pImageView) -> void;
    auto destroyImpl(SwapChain* pSwapchain) -> void;
    auto destroyImpl(Sampler* pSampler) -> void;
    auto destroyImpl(ShaderProgram* pProgram) -> void;
    auto destroyImpl(DescriptorSetLayout* pSetLayout) -> void;
    auto destroyImpl(PipelineLayout* pLayout) -> void;
    auto destroyImpl(QueryPool* pQueryPool) -> void;

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
        ThreadSafeObjectPool<QueryPool> queryPool;
        SyncPrimitiveAllocator syncPrimitive;
        std::unique_ptr<QueryPoolAllocator> queryPoolAllocator;
        std::unique_ptr<BindlessResource> bindless;

        explicit ResourcePool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;
};

// Implementation file for Device template methods
template <>
struct ResourceTraits<BufferCreateInfo>
{
    using ResourceType = Buffer;
};

template <>
struct ResourceTraits<ImageCreateInfo>
{
    using ResourceType = Image;
};

template <>
struct ResourceTraits<ImageViewCreateInfo>
{
    using ResourceType = ImageView;
};

template <>
struct ResourceTraits<SamplerCreateInfo>
{
    using ResourceType = Sampler;
};

template <>
struct ResourceTraits<ProgramCreateInfo>
{
    using ResourceType = ShaderProgram;
};

template <>
struct ResourceTraits<DescriptorSetLayoutCreateInfo>
{
    using ResourceType = DescriptorSetLayout;
};

template <>
struct ResourceTraits<PipelineLayoutCreateInfo>
{
    using ResourceType = PipelineLayout;
};

template <>
struct ResourceTraits<SwapChainCreateInfo>
{
    using ResourceType = SwapChain;
};

template <>
struct ResourceTraits<QueryPoolCreateInfo>
{
    using ResourceType = QueryPool;
};

// Main resource handling templates
template <typename TCreateInfo, typename TResource, typename TDebugName>
inline auto Device::create(TCreateInfo&& createInfo, TDebugName&& debugName, const std::source_location& location)
    -> Expected<TResource*>
{
    auto result = createImpl(APH_FWD(createInfo));

    if (result.success())
    {
        // Get name if possible
        std::string name;
        if constexpr (std::is_convertible_v<TDebugName, std::string>)
        {
            name = APH_FWD(debugName);
        }

        APH_VERIFY_RESULT(setDebugObjectName(result.value(), name));

        if (getCreateInfo().enableResourceTracking)
        {
            m_resourceStats.trackCreation<TResource>(location);
        }
    }

    return result;
}

template <typename TResource>
inline auto Device::destroy(TResource* pResource, const std::source_location& location) -> void
{
    APH_ASSERT(pResource != nullptr);
    
    if (getCreateInfo().enableResourceTracking)
    {
        m_resourceStats.trackDestruction<TResource>(location);
    }
    
    destroyImpl(pResource);
}

// Debug name setter templates
template <ResourceHandleType TObject>
inline auto Device::setDebugObjectName(TObject* object, auto&& name) -> Result
{
    object->setDebugName(APH_FWD(name));
    auto handle = object->getHandle();
    if constexpr (!std::is_same_v<DummyHandle, decltype(handle)>)
    {
        return setDebugObjectName(handle, object->getDebugName());
    }
    return Result::Success;
}

template <typename TObject>
    requires(!ResourceHandleType<TObject>)
inline auto Device::setDebugObjectName(TObject object, std::string_view name) -> Result
{
    ::vk::DebugUtilsObjectNameInfoEXT info{};
    info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
        .setObjectType(object.objectType)
        .setPObjectName(name.data());
    return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
}

inline auto Device::getResourceStatsReport() const -> std::string
{
    return m_resourceStats.generateReport();
}

} // namespace aph::vk
