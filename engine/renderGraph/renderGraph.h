#pragma once

#include "api/vulkan/device.h"
#include "renderPass.h"
#include "threads/taskManager.h"
#include <variant>

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

    template <typename T>
    T* getResource(const std::string& name);

private:
    friend class RenderPass;
    using ResourcePtr = std::variant<vk::Buffer*, vk::Image*>;
    PassResource* getPassResource(const std::string& name) const;
    PassResource* createPassResource(const std::string& name, PassResource::Type type);
    PassResource* importPassResource(const std::string& name, ResourcePtr resource);

    void setupImageResource(PassImageResource* imageResource, bool isColorAttachment);

    void setupImageBarrier(SmallVector<vk::ImageBarrier>& barriers, PassImageResource* resource,
                           ResourceState newState);

    template <typename BarrierType, typename ResourceType>
    void setupResourceBarrier(SmallVector<BarrierType>& barriers, ResourceType* resource, ResourceState targetState);

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

        // Resource state tracking at graph level
        HashMap<PassResource*, ResourceState> currentResourceStates;

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

template <typename T>
T* RenderGraph::getResource(const std::string& name)
{
    auto* resource = getPassResource(name);
    if constexpr (std::is_same_v<std::decay_t<T>, vk::Image>)
    {
        if (auto it = m_buildData.image.find(resource); it != m_buildData.image.end())
        {
            return it->second;
        }
    }
    else if constexpr (std::is_same_v<std::decay_t<T>, vk::Buffer>)
    {
        if (auto it = m_buildData.buffer.find(resource); it != m_buildData.buffer.end())
        {
            return it->second;
        }
    }

    CM_LOG_ERR("Could not find the pass resource [%s].");
    APH_ASSERT(false);
    return nullptr;
}
} // namespace aph
