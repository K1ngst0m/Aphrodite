#pragma once

#include "api/vulkan/device.h"
#include "common/breadcrumbTracker.h"
#include "common/result.h"
#include "exception/errorMacros.h"
#include "renderPass.h"
#include "resource/resourceLoader.h"
#include "threads/taskManager.h"
#include <variant>

GENERATE_LOG_FUNCS(RDG)

namespace aph
{
class FrameComposer;

class RenderGraph
{
public:
    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph(RenderGraph&&)                 = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph& operator=(RenderGraph&&)      = delete;

private:
    explicit RenderGraph(vk::Device* pDevice);
    RenderGraph();
    ~RenderGraph();

    auto initialize(vk::Device* pDevice) -> Result;
    auto initialize() -> Result; // For dry run mode

public:
    // Factory methods
    static auto Create(vk::Device* pDevice) -> Expected<RenderGraph*>;
    static auto CreateDryRun() -> Expected<RenderGraph*>;
    static void Destroy(RenderGraph* pGraph);

public:
    auto createPass(const std::string& name, QueueType queueType) -> RenderPass*;
    auto getPass(const std::string& name) const noexcept -> RenderPass*;
    void setBackBuffer(const std::string& backBuffer);
    template <typename T>
    auto getResource(const std::string& name) -> T*;

    void build(vk::SwapChain* pSwapChain = nullptr);
    void execute(vk::Fence** ppFence = {});
    void cleanup();

public:
    auto exportToGraphviz() const -> std::string;

    void enableDebugOutput(bool enable);
    void setForceDryRun(bool value);

    // Breadcrumb tracking methods
    auto getBreadcrumbTracker() -> BreadcrumbTracker&;
    auto generateBreadcrumbReport() const -> std::string;

public:
    class PassGroup
    {
        RenderGraph* m_graph;
        std::string m_groupName;
        std::vector<RenderPass*> m_passes;

    public:
        PassGroup(RenderGraph* graph, std::string name)
            : m_graph(graph)
            , m_groupName(std::move(name))
        {
        }

        auto addPass(const std::string& name, QueueType queueType) -> RenderPass*;
        void addPass(RenderPass* pass);
        auto getPasses() -> std::vector<RenderPass*>&;
        auto getName() const -> const std::string&;
    };

    auto createPassGroup(const std::string& name) -> PassGroup;

public:
    struct DebugCaptureInfo
    {
        bool enabled = false;
        std::string outputPath;
        std::vector<std::string> capturePassNames;
    };

    void enableFrameCapture(const std::string& outputPath);
    void addPassToCapture(const std::string& passName);

private:
    auto isDryRunMode() const -> bool;
    auto isDebugOutputEnabled() const -> bool;

    // Breadcrumb tracking helpers
    void initializeBreadcrumbTracker();
    void recordPassBreadcrumb(RenderPass* pass, uint32_t passIndex, bool start = true);
    void integrateCommandBufferBreadcrumbs(RenderPass* pass, vk::CommandBuffer* cmd);

private:
    friend class RenderPass;
    friend class FrameComposer;
    using ResourcePtr = std::variant<vk::Buffer*, vk::Image*>;
    auto getPassResource(const std::string& name) const -> PassResource*;
    auto createPassResource(const std::string& name, PassResource::Type type) -> PassResource*;
    auto importPassResource(const std::string& name, ResourcePtr resource) -> PassResource*;
    void importShader(const std::string& name, vk::ShaderProgram* pProgram);

    void setupImageResource(PassImageResource* imageResource, bool isColorAttachment);

    void setupImageBarrier(SmallVector<vk::ImageBarrier>& barriers, PassImageResource* resource,
                           ResourceState newState);

    template <typename BarrierType, typename ResourceType>
    void setupResourceBarrier(SmallVector<BarrierType>& barriers, ResourceType* resource, ResourceState targetState);

private:
    // Dirty flags to track what needs to be rebuilt
    enum DirtyFlagBits : uint32_t
    {
        None                = 0,
        PassDirty           = 1 << 0, // Render passes changed
        ImageResourceDirty  = 1 << 1, // Image resources changed
        BufferResourceDirty = 1 << 2, // Buffer resources changed
        TopologyDirty       = 1 << 3, // Graph topology changed
        BackBufferDirty     = 1 << 4, // Back buffer changed
        SwapChainDirty      = 1 << 5, // Swapchain changed
        All                 = 0xFFFFFFFF // Everything is dirty
    };

    using DirtyFlags        = uint32_t;
    DirtyFlags m_dirtyFlags = DirtyFlagBits::All;

    void clearDirtyFlags();
    auto isDirty(DirtyFlags flags) const -> bool;
    void setDirty(DirtyFlags flags);

    void markResourcesChanged(PassResource::Type type);
    void markPassModified();
    void markImageResourcesModified();
    void markBufferResourcesModified();
    void markBackBufferModified();
    void markTopologyModified();

private:
    vk::Device* m_pDevice                                 = {}; // Will be nullptr in dry run mode
    vk::CommandBufferAllocator* m_pCommandBufferAllocator = {};
    BreadcrumbTracker m_breadcrumbs; // Frame-level breadcrumb tracker

    // Pending resource loads
    struct PendingBufferLoad
    {
        std::string name;
        BufferLoadInfo loadInfo;
        BufferUsage usage;
        PassBufferResource* resource;
        ResourceLoadCallback preCallback;
        ResourceLoadCallback postCallback;
    };

    struct PendingImageLoad
    {
        std::string name;
        ImageLoadInfo loadInfo;
        ImageUsage usage;
        PassImageResource* resource;
        ResourceLoadCallback preCallback;
        ResourceLoadCallback postCallback;
    };

    struct PendingShaderLoad
    {
        std::string name;
        ShaderLoadInfo loadInfo;
        ResourceLoadCallback preCallback;
        ResourceLoadCallback postCallback;
    };

    struct
    {
        std::string backBuffer = {};
        HashMap<std::string, RenderPass*> passMap;
        HashMap<std::string, PassResource*> resourceMap;
        HashMap<std::string, PendingBufferLoad> pendingBufferLoad;
        HashMap<std::string, PendingImageLoad> pendingImageLoad;
        HashMap<std::string, PendingShaderLoad> pendingShaderLoad;
    } m_declareData;

    struct
    {
        HashMap<RenderPass*, HashSet<RenderPass*>> passDependencyGraph;
        SmallVector<RenderPass*> sortedPasses;

        HashMap<RenderPass*, vk::CommandBuffer*> cmds;
        HashMap<RenderPass*, SmallVector<vk::ImageBarrier>> imageBarriers;
        HashMap<RenderPass*, SmallVector<vk::BufferBarrier>> bufferBarriers;

        HashMap<PassResource*, vk::Image*> image;
        HashMap<PassResource*, vk::Buffer*> buffer;
        HashMap<std::string, vk::ShaderProgram*> program;

        // Resource state tracking at graph level
        HashMap<PassResource*, ResourceState> currentResourceStates;

        vk::SwapChain* pSwapchain    = {};
        vk::Fence* frameExecuteFence = {};

        SmallVector<vk::QueueSubmitInfo> frameSubmitInfos{};
        std::mutex submitLock;

        // Breadcrumb indices for each pass
        HashMap<RenderPass*, uint32_t> passBreadcrumbIndices;
    } m_buildData;

    struct
    {
        ThreadSafeObjectPool<PassBufferResource> passBufferResource;
        ThreadSafeObjectPool<PassImageResource> passImageResource;
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;

    // Debug output for dry run mode
    bool m_debugOutputEnabled = false;
    bool m_forceDryRun        = false;

    struct TransientResourceInfo
    {
        uint32_t firstUsePassIndex = UINT32_MAX;
        uint32_t lastUsePassIndex  = 0;
        size_t size                = 0;
        bool isImage               = false;
    };

    HashMap<PassResource*, TransientResourceInfo> m_transientResources;

    void analyzeResourceLifetimes();
    auto isResourceTransient(PassResource* resource) const -> bool;

    DebugCaptureInfo m_debugCapture;
    void capturePassOutput(RenderPass* pass, vk::CommandBuffer* cmd);
};

// Template implementations
template <typename T>
inline auto RenderGraph::getResource(const std::string& name) -> T*
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

    RDG_LOG_ERR("Could not find the pass resource [%s].", name.c_str());
    APH_ASSERT(false);
    return nullptr;
}

// PassGroup inline implementations
inline auto RenderGraph::PassGroup::addPass(const std::string& name, QueueType queueType) -> RenderPass*
{
    auto* pass = m_graph->createPass(name, queueType);
    m_passes.push_back(pass);
    return pass;
}

inline void RenderGraph::PassGroup::addPass(RenderPass* pass)
{
    m_passes.push_back(pass);
}

inline auto RenderGraph::PassGroup::getPasses() -> std::vector<RenderPass*>&
{
    return m_passes;
}

inline auto RenderGraph::PassGroup::getName() const -> const std::string&
{
    return m_groupName;
}

// RenderGraph inline implementations
inline auto RenderGraph::createPassGroup(const std::string& name) -> PassGroup
{
    return PassGroup{ this, name };
}

inline void RenderGraph::enableDebugOutput(bool enable)
{
    m_debugOutputEnabled = enable;
}

inline void RenderGraph::setForceDryRun(bool value)
{
    m_forceDryRun = value;
}

inline void RenderGraph::enableFrameCapture(const std::string& outputPath)
{
    m_debugCapture.enabled    = true;
    m_debugCapture.outputPath = outputPath;
}

inline void RenderGraph::addPassToCapture(const std::string& passName)
{
    m_debugCapture.capturePassNames.push_back(passName);
}

inline auto RenderGraph::isDryRunMode() const -> bool
{
    return m_pDevice == nullptr || m_forceDryRun;
}

inline auto RenderGraph::isDebugOutputEnabled() const -> bool
{
    return m_debugOutputEnabled;
}

inline void RenderGraph::importShader(const std::string& name, vk::ShaderProgram* pProgram)
{
    APH_ASSERT(pProgram);
    m_buildData.program[name] = pProgram;
}

inline void RenderGraph::clearDirtyFlags()
{
    m_dirtyFlags = DirtyFlagBits::None;
}

inline auto RenderGraph::isDirty(DirtyFlags flags) const -> bool
{
    return (m_dirtyFlags & flags) != 0;
}

inline void RenderGraph::setDirty(DirtyFlags flags)
{
    m_dirtyFlags |= flags;
}

inline void RenderGraph::markResourcesChanged(PassResource::Type type)
{
    if (type == PassResource::Type::eImage)
    {
        markImageResourcesModified();
    }
    else if (type == PassResource::Type::eBuffer)
    {
        markBufferResourcesModified();
    }
    markTopologyModified();
}

inline void RenderGraph::markPassModified()
{
    setDirty(DirtyFlagBits::PassDirty | DirtyFlagBits::TopologyDirty);
}

inline void RenderGraph::markImageResourcesModified()
{
    setDirty(DirtyFlagBits::ImageResourceDirty);
}

inline void RenderGraph::markBufferResourcesModified()
{
    setDirty(DirtyFlagBits::BufferResourceDirty);
}

inline void RenderGraph::markBackBufferModified()
{
    setDirty(DirtyFlagBits::BackBufferDirty);
}

inline void RenderGraph::markTopologyModified()
{
    setDirty(DirtyFlagBits::TopologyDirty);
}

// Breadcrumb tracking inline implementations
inline auto RenderGraph::getBreadcrumbTracker() -> BreadcrumbTracker&
{
    return m_breadcrumbs;
}

} // namespace aph
