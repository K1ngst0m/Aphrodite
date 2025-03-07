#pragma once

#include "bindless.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "common/timer.h"
#include "descriptorSet.h"
#include "image.h"
#include "instance.h"
#include "module/module.h"
#include "physicalDevice.h"
#include "queue.h"
#include "sampler.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitive.h"

namespace aph::vk
{
class DeviceAllocator;

struct DeviceCreateInfo
{
    GPUFeature enabledFeatures = {};
    PhysicalDevice* pPhysicalDevice = nullptr;
    Instance* pInstance = nullptr;
    bool enableCapture = false;
};

class Device : public ResourceHandle<::vk::Device, DeviceCreateInfo>
{
private:
    Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle);

public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo& createInfo);
    static void Destroy(Device* pDevice);

public:
    Result create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, const std::string& debugName = "");
    Result create(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const std::string& debugName = "");
    Result create(const ImageCreateInfo& createInfo, Image** ppImage, const std::string& debugName = "");
    Result create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, const std::string& debugName = "");
    Result create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, const std::string& debugName = "");
    Result create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, const std::string& debugName = "");
    Result create(const ShaderCreateInfo& createInfo, Shader** ppShader, const std::string& debugName = "");
    Result create(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout,
                  const std::string& debugName = "");
    Result create(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool,
                  const std::string& debugName = "");

    void destroy(Buffer* pBuffer);
    void destroy(Image* pImage);
    void destroy(ImageView* pImageView);
    void destroy(SwapChain* pSwapchain);
    void destroy(Sampler* pSampler);
    void destroy(ShaderProgram* pProgram);
    void destroy(Shader* pShader);
    void destroy(DescriptorSetLayout* pSetLayout);
    void destroy(CommandPool* pPool);

public:
    DeviceAddress getDeviceAddress(Buffer* pBuffer) const
    {
        ::vk::DeviceAddress address =
            getHandle().getBufferAddress(::vk::BufferDeviceAddressInfo{ pBuffer->getHandle() });
        return static_cast<DeviceAddress>(address);
    }
    BindlessResource* getBindlessResource(ShaderProgram* pProgram) const
    {
        APH_ASSERT(m_resourcePool.bindless.contains(pProgram));
        return m_resourcePool.bindless.at(pProgram).get();
    }
    Result waitIdle();
    Result waitForFence(ArrayProxy<Fence*> fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

    Semaphore* acquireSemaphore();
    Fence* acquireFence(bool isSignaled);
    Result releaseSemaphore(Semaphore* semaphore);
    Result releaseFence(Fence* pFence);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeCommand(Queue* queue, const CmdRecordCallBack&& func, std::vector<Semaphore*> waitSems = {},
                        std::vector<Semaphore*> signalSems = {}, Fence* pFence = nullptr);

public:
    Result flushMemory(Image* pImage, Range range = {});
    Result flushMemory(Buffer* pBuffer, Range range = {});
    Result invalidateMemory(Image* pImage, Range range = {});
    Result invalidateMemory(Buffer* pBuffer, Range range = {});

    void* mapMemory(Buffer* pBuffer) const;
    void unMapMemory(Buffer* pBuffer) const;

public:
    PhysicalDevice* getPhysicalDevice() const
    {
        return getCreateInfo().pPhysicalDevice;
    }
    GPUFeature getEnabledFeatures() const
    {
        return getCreateInfo().enabledFeatures;
    }
    Format getDepthFormat() const;
    Queue* getQueue(QueueType type, uint32_t queueIndex = 0);

    double getTimeQueryResults(::vk::QueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                               TimeUnit unitType = TimeUnit::Seconds);

public:
    ::vk::PipelineStageFlags determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType);

    template <ResourceHandleType TObject>
    Result setDebugObjectName(TObject* object, const std::string& name)
    {
        object->setDebugName(name);
        auto handle = object->getHandle();
        return setDebugObjectName(handle, name);
    }

    template <typename TObject>
        requires(!ResourceHandleType<TObject>)
    Result setDebugObjectName(TObject object, const std::string& name)
    {
        ::vk::DebugUtilsObjectNameInfoEXT info{};
        info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
            .setObjectType(object.objectType)
            .setPObjectName(name.data());
        return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
    }

public:
    void begineCapture();
    void endCapture();
    void triggerCapture();

private:
    Result initCapture();
    Module m_renderdocModule{};

private:
    HashMap<QueueType, SmallVector<Queue*>> m_queues;

private:
    struct ResourceObjectPool
    {
        DeviceAllocator* deviceMemory;
        ThreadSafeObjectPool<Buffer> buffer;
        ThreadSafeObjectPool<Image> image;
        ThreadSafeObjectPool<Sampler> sampler;
        ThreadSafeObjectPool<ImageView> imageView;
        ThreadSafeObjectPool<DescriptorSetLayout> setLayout;
        ThreadSafeObjectPool<ShaderProgram> program;
        ThreadSafeObjectPool<Queue> queue;
        ThreadSafeObjectPool<CommandPool> commandPool;
        ThreadSafeObjectPool<Shader> shader;
        SyncPrimitiveAllocator syncPrimitive;
        HashMap<ShaderProgram*, std::unique_ptr<BindlessResource>> bindless;

        ResourceObjectPool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;
};

} // namespace aph::vk
