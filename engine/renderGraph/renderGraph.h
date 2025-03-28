#pragma once

#include "renderPass.h"
#include "api/vulkan/device.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph
{
public:
    RenderGraph(vk::Device* pDevice);
    ~RenderGraph();

    RenderPass* createPass(const std::string& name, QueueType queueType);
    RenderPass* getPass(const std::string& name);

    PassResource* importResource(const std::string& name, vk::Image* pImage);
    PassResource* importResource(const std::string& name, vk::Buffer* pBuffer);
    PassResource* getResource(const std::string& name, PassResource::Type type);
    bool hasResource(const std::string& name) const
    {
        return m_declareData.resourceMap.contains(name);
    }
    vk::Image* getBuildResource(PassImageResource* pResource) const;
    vk::Buffer* getBuildResource(PassBufferResource* pResource) const;

    void setBackBuffer(const std::string& backBuffer);

    void build(vk::SwapChain* pSwapChain = nullptr);
    void execute(vk::Fence* pFence = nullptr);
    void cleanup();

private:
    vk::Device* m_pDevice = {};

    struct
    {
        std::string backBuffer = {};

        SmallVector<RenderPass*> passes;
        HashMap<std::string, std::size_t> passMap;

        SmallVector<PassBufferResource*> bufferResources;
        SmallVector<PassImageResource*> imageResources;
        SmallVector<PassResource*> resources;
        HashMap<std::string, std::size_t> resourceMap;
    } m_declareData;

    struct
    {
        SmallVector<RenderPass*> sortedPasses;

        HashMap<RenderPass*, vk::CommandPool*> cmdPools;
        HashMap<RenderPass*, vk::CommandBuffer*> cmds;
        HashMap<RenderPass*, HashSet<RenderPass*>> passDependencyGraph;
        HashMap<RenderPass*, std::vector<vk::ImageBarrier>> imageBarriers;
        HashMap<RenderPass*, std::vector<vk::BufferBarrier>> bufferBarriers;

        HashMap<PassResource*, vk::Image*> image;
        HashMap<PassResource*, vk::Buffer*> buffer;

        vk::SwapChain* pSwapchain = {};
        vk::Fence* frameFence = {};

        std::vector<vk::QueueSubmitInfo> frameSubmitInfos{};
        std::mutex submitLock;

    } m_buildData;

    struct
    {
        ThreadSafeObjectPool<PassBufferResource> passBufferResource;
        ThreadSafeObjectPool<PassImageResource> passImageResource;
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;
};

} // namespace aph
