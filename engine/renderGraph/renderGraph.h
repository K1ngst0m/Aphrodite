#pragma once

#include "api/vulkan/device.h"
#include "renderPass.h"
#include "threads/taskManager.h"

namespace aph
{
class RenderGraph
{
public:
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph& operator=(RenderGraph&&) = delete;
    RenderGraph(vk::Device* pDevice);
    ~RenderGraph();

    RenderPass* createPass(const std::string& name, QueueType queueType);
    void setBackBuffer(const std::string& backBuffer);

    void build(vk::SwapChain* pSwapChain = nullptr);
    void execute(vk::Fence* pFence = nullptr);
    void cleanup();

    std::string exportToGraphviz() const;

private:
    friend class RenderPass;
    PassResource* importResource(const std::string& name, vk::Image* pImage);
    PassResource* importResource(const std::string& name, vk::Buffer* pBuffer);
    PassResource* getResource(const std::string& name, PassResource::Type type);

private:
    // Dirty flags to track what needs to be rebuilt
    enum DirtyFlagBits : uint32_t
    {
        None = 0,
        PassDirty = 1 << 0, // Render passes changed
        ImageResourceDirty = 1 << 1, // Image resources changed
        BufferResourceDirty = 1 << 2, // Buffer resources changed
        TopologyDirty = 1 << 3, // Graph topology changed
        BackBufferDirty = 1 << 4, // Back buffer changed
        SwapChainDirty = 1 << 5, // Swapchain changed
        All = 0xFFFFFFFF // Everything is dirty
    };
    using DirtyFlags = uint32_t;
    DirtyFlags m_dirtyFlags = DirtyFlagBits::All;

    void clearDirtyFlags()
    {
        m_dirtyFlags = DirtyFlagBits::None;
    }
    bool isDirty(DirtyFlags flags) const
    {
        return (m_dirtyFlags & flags) != 0;
    }
    void setDirty(DirtyFlags flags)
    {
        m_dirtyFlags |= flags;
    }

private:
    vk::Device* m_pDevice = {};

    struct
    {
        std::string backBuffer = {};
        HashMap<std::string, RenderPass*> passMap;
        HashMap<std::string, PassResource*> resourceMap;
    } m_declareData;

    struct
    {
        HashMap<RenderPass*, HashSet<RenderPass*>> passDependencyGraph;
        SmallVector<RenderPass*> sortedPasses;

        HashMap<RenderPass*, vk::CommandPool*> cmdPools;
        HashMap<RenderPass*, vk::CommandBuffer*> cmds;
        HashMap<RenderPass*, SmallVector<vk::ImageBarrier>> imageBarriers;
        HashMap<RenderPass*, SmallVector<vk::BufferBarrier>> bufferBarriers;

        HashMap<PassResource*, vk::Image*> image;
        HashMap<PassResource*, vk::Buffer*> buffer;

        vk::SwapChain* pSwapchain = {};
        vk::Fence* frameFence = {};

        SmallVector<vk::QueueSubmitInfo> frameSubmitInfos{};
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
