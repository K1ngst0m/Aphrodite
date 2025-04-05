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
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"

#include "api/deviceAllocator.h"
#include "module/module.h"
#include "exception/exception.h"

namespace aph::vk
{
struct DeviceCreateInfo
{
    GPUFeature enabledFeatures = {};
    PhysicalDevice* pPhysicalDevice = nullptr;
    Instance* pInstance = nullptr;
};

// Type traits to map CreateInfo types to Resource types
template <typename TCreateInfo>
struct ResourceTraits;

template <>
struct ResourceTraits<BufferCreateInfo> {
    using ResourceType = Buffer*;
};

template <>
struct ResourceTraits<ImageCreateInfo> {
    using ResourceType = Image*;
};

template <>
struct ResourceTraits<ImageViewCreateInfo> {
    using ResourceType = ImageView*;
};

template <>
struct ResourceTraits<SamplerCreateInfo> {
    using ResourceType = Sampler*;
};

template <>
struct ResourceTraits<ProgramCreateInfo> {
    using ResourceType = ShaderProgram*;
};

template <>
struct ResourceTraits<DescriptorSetLayoutCreateInfo> {
    using ResourceType = DescriptorSetLayout*;
};

template <>
struct ResourceTraits<PipelineLayoutCreateInfo> {
    using ResourceType = PipelineLayout*;
};

template <>
struct ResourceTraits<SwapChainCreateInfo> {
    using ResourceType = SwapChain*;
};

class Device : public ResourceHandle<::vk::Device, DeviceCreateInfo>
{
private:
    Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle);

public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo& createInfo);
    static void Destroy(Device* pDevice);

public:
    template <typename TCreateInfo,
              typename TResource = typename ResourceTraits<std::decay_t<TCreateInfo>>::ResourceType,
              typename TDebugName = std::string>
    Expected<TResource> create(TCreateInfo&& createInfo, TDebugName&& debugName = {});

    template <typename TResource>
    void destroy(TResource* pResource);

public:
    DeviceAddress getDeviceAddress(Buffer* pBuffer) const;
    BindlessResource* getBindlessResource() const;
    CommandBufferAllocator* getCommandBufferAllocator() const
    {
        return m_resourcePool.commandBufferAllocator.get();
    }
    Result waitIdle();
    Result waitForFence(ArrayProxy<Fence*> fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

    Semaphore* acquireSemaphore();
    Fence* acquireFence(bool isSignaled);
    Result releaseSemaphore(Semaphore* semaphore);
    Result releaseFence(Fence* pFence);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems = {},
                        ArrayProxy<Semaphore*> signalSems = {}, Fence* pFence = nullptr);

public:
    Result flushMemory(Image* pImage, Range range = {});
    Result flushMemory(Buffer* pBuffer, Range range = {});
    Result invalidateMemory(Image* pImage, Range range = {});
    Result invalidateMemory(Buffer* pBuffer, Range range = {});

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

private:
    Expected<Sampler*> createImpl(const SamplerCreateInfo& createInfo);
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
    struct ResourcePool
    {
        std::unique_ptr<DeviceAllocator> deviceMemory;
        std::unique_ptr<CommandBufferAllocator> commandBufferAllocator;
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

        ResourcePool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;
};
template <typename TCreateInfo, typename TResource, typename TDebugName>
inline Expected<TResource> Device::create(TCreateInfo&& createInfo, TDebugName&& debugName)
{
    auto result = createImpl(APH_FWD(createInfo));
    if (result.success())
    {
        APH_VERIFY_RESULT(setDebugObjectName(result.value(), APH_FWD(debugName)));
    }
    return result;
}

template <typename TResource>
inline void Device::destroy(TResource* pResource)
{
    CM_LOG_DEBUG("Destroy resource: %s", pResource->getDebugName());
    destroyImpl(pResource);
}

template <ResourceHandleType TObject>
inline Result Device::setDebugObjectName(TObject* object, auto&& name)
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
inline Result Device::setDebugObjectName(TObject object, std::string_view name)
{
    ::vk::DebugUtilsObjectNameInfoEXT info{};
    info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
        .setObjectType(object.objectType)
        .setPObjectName(name.data());
    return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
}
} // namespace aph::vk
