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

// ResourceStats implementation
inline auto ResourceStats::resourceTypeToString(ResourceType type) -> const char*
{
    static constexpr std::array kTypeNames = {
        "Buffer",
        "Image",
        "ImageView",
        "Sampler",
        "ShaderProgram",
        "DescriptorSetLayout",
        "PipelineLayout",
        "SwapChain",
        "CommandBuffer",
        "Queue",
        "Fence",
        "Semaphore"
    };
    
    return kTypeNames[static_cast<size_t>(type)];
}

inline auto ResourceStats::trackCreation(ResourceType type) -> void
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_created[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]++;
}

inline auto ResourceStats::trackDestruction(ResourceType type) -> void
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_destroyed[static_cast<size_t>(type)]++;
    m_active[static_cast<size_t>(type)]--;
}

inline auto ResourceStats::generateReport() const -> std::string
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;
    
    ss << "Resource Usage Report:\n";
    ss << "--------------------------------------------------\n";
    ss << "Type                 | Created | Destroyed | Active\n";
    ss << "--------------------------------------------------\n";
    
    bool hasLeaks = false;
    std::stringstream leakReport;
    leakReport << "\nPotential Resource Leaks:\n";
    leakReport << "--------------------------------------------------\n";
    leakReport << "Type                 | Leaked | % of Created\n";
    leakReport << "--------------------------------------------------\n";
    
    uint32_t totalCreated = 0;
    uint32_t totalDestroyed = 0;
    uint32_t totalLeaked = 0;
    
    for (size_t i = 0; i < static_cast<size_t>(ResourceType::eCount); i++)
    {
        auto type = static_cast<ResourceType>(i);
        uint32_t created = m_created[i];
        uint32_t destroyed = m_destroyed[i];
        uint32_t active = m_active[i];
        
        // Add to totals
        totalCreated += created;
        totalDestroyed += destroyed;
        
        // Output standard resource report
        ss << std::left << std::setw(20) << resourceTypeToString(type) << " | "
           << std::right << std::setw(7) << created << " | "
           << std::setw(9) << destroyed << " | "
           << std::setw(6) << active << "\n";
        
        // Check for leaks and add to leak report if found
        if (active > 0 && created > 0)
        {
            hasLeaks = true;
            totalLeaked += active;
            double leakPercentage = (static_cast<double>(active) / created) * 100.0;
            
            leakReport << std::left << std::setw(20) << resourceTypeToString(type) << " | "
                       << std::right << std::setw(6) << active << " | "
                       << std::fixed << std::setprecision(1) << std::setw(6) << leakPercentage << "%\n";
        }
    }
    
    ss << "--------------------------------------------------\n";
    ss << "Total                | " 
       << std::right << std::setw(7) << totalCreated << " | "
       << std::setw(9) << totalDestroyed << " | "
       << std::setw(6) << totalLeaked << "\n";
    ss << "--------------------------------------------------\n";
    
    // Add leak report if leaks were found
    if (hasLeaks)
    {
        double overallLeakPercentage = (static_cast<double>(totalLeaked) / totalCreated) * 100.0;
        
        leakReport << "--------------------------------------------------\n";
        leakReport << "Total Resources Leaked: " << totalLeaked << " (" 
                   << std::fixed << std::setprecision(1) << overallLeakPercentage 
                   << "% of all created resources)\n";
        leakReport << "--------------------------------------------------\n";
        
        ss << "\n" << leakReport.str();
    }
    else
    {
        ss << "\nNo Resource Leaks Detected!\n";
    }
    
    return ss.str();
}

inline auto ResourceStats::getCreatedCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_created[static_cast<size_t>(type)]; 
}

inline auto ResourceStats::getDestroyedCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destroyed[static_cast<size_t>(type)]; 
}

inline auto ResourceStats::getActiveCount(ResourceType type) const -> uint32_t
{ 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active[static_cast<size_t>(type)]; 
}
template <typename T>
inline auto GetResourceType() -> ResourceStats::ResourceType
{
    static_assert(dependent_false_v<T>, "unsupported resource type.");
}

// Template specializations for resource type mapping
template<> inline auto GetResourceType<Buffer>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eBuffer; }
template<> inline auto GetResourceType<Image>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eImage; }
template<> inline auto GetResourceType<ImageView>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eImageView; }
template<> inline auto GetResourceType<Sampler>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSampler; }
template<> inline auto GetResourceType<ShaderProgram>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eShaderProgram; }
template<> inline auto GetResourceType<DescriptorSetLayout>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eDescriptorSetLayout; }
template<> inline auto GetResourceType<PipelineLayout>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::ePipelineLayout; }
template<> inline auto GetResourceType<SwapChain>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSwapChain; }
template<> inline auto GetResourceType<CommandBuffer>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eCommandBuffer; }
template<> inline auto GetResourceType<Queue>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eQueue; }
template<> inline auto GetResourceType<Fence>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eFence; }
template<> inline auto GetResourceType<Semaphore>() -> ResourceStats::ResourceType { return ResourceStats::ResourceType::eSemaphore; }

// Resource tracking templates
template <typename TResource>
inline auto Device::trackResourceCreation() -> void
{
    m_resourceStats.trackCreation(GetResourceType<TResource>());
}

template <typename TResource>
inline auto Device::trackResourceDestruction() -> void
{
    m_resourceStats.trackDestruction(GetResourceType<TResource>());
}

// Main resource handling templates
template <typename TCreateInfo, typename TResource, typename TDebugName>
inline auto Device::create(TCreateInfo&& createInfo, TDebugName&& debugName,
                           const std::source_location&  /*location*/) -> Expected<TResource*>
{
    auto result = createImpl(APH_FWD(createInfo));
    
    if (result.success())
    {
        // Get name if possible
        std::string name;
        if constexpr(std::is_convertible_v<TDebugName, std::string>)
        {
            name = std::forward<TDebugName>(debugName);
        }
        
        // Set object debug name if provided
        if (!name.empty())
        {
            APH_VERIFY_RESULT(setDebugObjectName(result.value(), name));
        }
        
        // Track resource creation in stats
        trackResourceCreation<TResource>();
    }
    
    return result;
}

template <typename TResource>
inline auto Device::destroy(TResource* pResource, const std::source_location&  /*location*/) -> void
{
    if (pResource)
    {
        // Track resource destruction in stats
        trackResourceDestruction<TResource>();
        
        // Call implementation-specific destroy method
        destroyImpl(pResource);
    }
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

}
