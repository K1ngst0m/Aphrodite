#pragma once

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
    bool enableCapture = true;
};

class Device : public ResourceHandle<::vk::Device, DeviceCreateInfo>
{
private:
    Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle);

public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo& createInfo);
    static void Destroy(Device* pDevice);

public:
    Result create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, std::string_view debugName = "");
    Result create(const BufferCreateInfo& createInfo, Buffer** ppBuffer, std::string_view debugName = "");
    Result create(const ImageCreateInfo& createInfo, Image** ppImage, std::string_view debugName = "");
    Result create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, std::string_view debugName = "");
    Result create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, std::string_view debugName = "");
    Result create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, std::string_view debugName = "");
    Result create(const ShaderCreateInfo& createInfo, Shader** ppShader, std::string_view debugName = "");
    Result create(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout,
                  std::string_view debugName = "");
    Result create(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool,
                  std::string_view debugName = "");

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
    Semaphore* acquireSemaphore();
    Fence* acquireFence(bool isSignaled);
    Result releaseSemaphore(Semaphore* semaphore);
    Result releaseFence(Fence* pFence);

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeCommand(Queue* queue, const CmdRecordCallBack&& func, const std::vector<Semaphore*>& waitSems = {},
                        const std::vector<Semaphore*>& signalSems = {}, Fence* pFence = nullptr);

public:
    Result flushMemory(::vk::DeviceMemory memory, MemoryRange range = {});
    Result invalidateMemory(::vk::DeviceMemory memory, MemoryRange range = {});
    Result mapMemory(Buffer* pBuffer, void** ppMapped) const;
    void unMapMemory(Buffer* pBuffer) const;

public:
    PhysicalDevice* getPhysicalDevice() const
    {
        return m_gpu;
    }
    Format getDepthFormat() const;
    Queue* getQueue(QueueType type, uint32_t queueIndex = 0);

    double getTimeQueryResults(::vk::QueryPool pool, uint32_t firstQuery, uint32_t secondQuery,
                               TimeUnit unitType = TimeUnit::Seconds);

public:
    ::vk::PipelineStageFlags determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType);

    template <typename TObject>
    Result setDebugObjectName(TObject object, std::string_view name)
    {
        ::vk::DebugUtilsObjectNameInfoEXT info{};
        info.setObjectHandle(uint64_t(static_cast<TObject::CType>(object)))
            .setObjectType(object.objectType)
            .setPObjectName(name.data());
        return utils::getResult(getHandle().setDebugUtilsObjectNameEXT(info));
    }

public:
    Result waitIdle();
    Result waitForFence(const std::vector<Fence*>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);

public:
    void begineCapture();
    void endCapture();
    void triggerCapture();

private:
    Result initCapture();
    Module m_renderdocModule{};

private:
    PhysicalDevice* m_gpu{};

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
        ThreadSafeObjectPool<vk::Shader> shader;
        SyncPrimitiveAllocator syncPrimitive;

        ResourceObjectPool(Device* pDevice)
            : syncPrimitive(pDevice)
        {
        }
    } m_resourcePool;
};

} // namespace aph::vk
