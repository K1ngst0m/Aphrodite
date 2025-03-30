#pragma once

#include "bindless.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
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

#include "module/module.h"
#include "api/deviceAllocator.h"

namespace aph::vk
{
struct DeviceCreateInfo
{
    GPUFeature enabledFeatures = {};
    PhysicalDevice* pPhysicalDevice = nullptr;
    Instance* pInstance = nullptr;
};

class Device : public ResourceHandle<::vk::Device, DeviceCreateInfo>
{
private:
    Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle);

public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo& createInfo);
    static void Destroy(Device* pDevice);

public:
    template <typename TCreateInfo, typename TResource, typename TDebugName = std::string>
    Result create(TCreateInfo&& createInfo, TResource** ppResource, TDebugName&& debugName = {});

    template <typename TResource>
    void destroy(TResource* pResource);

public:
    DeviceAddress getDeviceAddress(Buffer* pBuffer) const;
    BindlessResource* getBindlessResource() const;
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

public:
    void begineCapture();
    void endCapture();
    void triggerCapture();

private:
    Result createImpl(const SamplerCreateInfo& createInfo, Sampler** ppSampler);
    Result createImpl(const BufferCreateInfo& createInfo, Buffer** ppBuffer);
    Result createImpl(const ImageCreateInfo& createInfo, Image** ppImage);
    Result createImpl(const ImageViewCreateInfo& createInfo, ImageView** ppImageView);
    Result createImpl(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain);
    Result createImpl(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram);
    Result createImpl(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool);
    Result createImpl(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout);

    void destroyImpl(Buffer* pBuffer);
    void destroyImpl(Image* pImage);
    void destroyImpl(ImageView* pImageView);
    void destroyImpl(SwapChain* pSwapchain);
    void destroyImpl(Sampler* pSampler);
    void destroyImpl(ShaderProgram* pProgram);
    void destroyImpl(DescriptorSetLayout* pSetLayout);
    void destroyImpl(CommandPool* pPool);

private:
    Result initCapture();
    Module m_renderdocModule{};

private:
    HashMap<QueueType, SmallVector<Queue*>> m_queues;

private:
    struct ResourcePool
    {
        std::unique_ptr<DeviceAllocator> deviceMemory;
        ThreadSafeObjectPool<Buffer> buffer;
        ThreadSafeObjectPool<Image> image;
        ThreadSafeObjectPool<Sampler> sampler;
        ThreadSafeObjectPool<ImageView> imageView;
        ThreadSafeObjectPool<DescriptorSetLayout> setLayout;
        ThreadSafeObjectPool<ShaderProgram> program;
        ThreadSafeObjectPool<Queue> queue;
        ThreadSafeObjectPool<CommandPool> commandPool;
        SyncPrimitiveAllocator syncPrimitive;
        std::unique_ptr<BindlessResource> bindless;

        ResourcePool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;
};

template <typename TCreateInfo, typename TResource, typename TDebugName>
Result Device::create(TCreateInfo&& createInfo, TResource** ppResource, TDebugName&& debugName)
{
    ResultGroup result = createImpl(APH_FWD(createInfo), ppResource);
    result += setDebugObjectName(*ppResource, APH_FWD(debugName));
    return result;
}

template <typename TResource>
void Device::destroy(TResource* pResource)
{
    CM_LOG_DEBUG("Destroy resource: %s", pResource->getDebugName());
    destroyImpl(pResource);
}

template <ResourceHandleType TObject>
Result Device::setDebugObjectName(TObject* object, auto&& name)
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
Result Device::setDebugObjectName(TObject object, std::string_view name)
{
    ::vk::DebugUtilsObjectNameInfoEXT info{};
    info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
        .setObjectType(object.objectType)
        .setPObjectName(name.data());
    return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
}
} // namespace aph::vk
