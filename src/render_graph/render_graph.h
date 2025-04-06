#pragma once

#include "api/vulkan/device.h"
#include "common/result.h"
#include "exception/errorMacros.h"
#include "render_pass.h"
#include "threads/taskManager.h"
#include <variant>

GENERATE_LOG_FUNCS(RDG)

namespace aph
{
class RenderGraph
{
public:
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph& operator=(RenderGraph&&) = delete;

private:
    // Private constructors - use static Create methods instead
    explicit RenderGraph(vk::Device* pDevice);
    RenderGraph();
    ~RenderGraph();
    
    Result initialize(vk::Device* pDevice);
    Result initialize(); // For dry run mode

public:
    // Factory methods
    static Expected<RenderGraph*> Create(vk::Device* pDevice);
    static Expected<RenderGraph*> CreateDryRun();
    static void Destroy(RenderGraph* pGraph);

public:
    RenderPass* createPass(const std::string& name, QueueType queueType);
    RenderPass* getPass(const std::string& name) const noexcept
    {
        if (m_declareData.passMap.contains(name))
        {
            return m_declareData.passMap.at(name);
        }
        RDG_LOG_WARN("Could not found pass [%s]", name);
        return nullptr;
    }
    void setBackBuffer(const std::string& backBuffer);
    template <typename T>
    T* getResource(const std::string& name);

    void build(vk::SwapChain* pSwapChain = nullptr);
    void execute(vk::Fence** ppFence = {});
    void cleanup();

public:
    std::string exportToGraphviz() const;

    void enableDebugOutput(bool enable)
    {
        m_debugOutputEnabled = enable;
    }

    void setForceDryRun(bool value)
    {
        m_forceDryRun = value;
    }

public:
    class PassGroup
    {
        RenderGraph* m_graph;
        std::string m_groupName;
        std::vector<RenderPass*> m_passes;

    public:
        PassGroup(RenderGraph* graph, const std::string& name)
            : m_graph(graph)
            , m_groupName(name)
        {
        }

        RenderPass* addPass(const std::string& name, QueueType queueType)
        {
            auto* pass = m_graph->createPass(name, queueType);
            m_passes.push_back(pass);
            return pass;
        }

        void addPass(RenderPass* pass)
        {
            m_passes.push_back(pass);
        }

        std::vector<RenderPass*>& getPasses()
        {
            return m_passes;
        }
        const std::string& getName() const
        {
            return m_groupName;
        }
    };

    PassGroup createPassGroup(const std::string& name)
    {
        return PassGroup{ this, name };
    }

public:
    struct DebugCaptureInfo
    {
        bool enabled = false;
        std::string outputPath;
        std::vector<std::string> capturePassNames;
    };

    void enableFrameCapture(const std::string& outputPath)
    {
        m_debugCapture.enabled = true;
        m_debugCapture.outputPath = outputPath;
    }

    void addPassToCapture(const std::string& passName)
    {
        m_debugCapture.capturePassNames.push_back(passName);
    }

private:
    bool isDryRunMode() const
    {
        return m_pDevice == nullptr || m_forceDryRun;
    }

    bool isDebugOutputEnabled() const
    {
        return m_debugOutputEnabled;
    }

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

    void markResourcesChanged(PassResource::Type type)
    {
        if (type == PassResource::Type::Image)
        {
            markImageResourcesModified();
        }
        else if (type == PassResource::Type::Buffer)
        {
            markBufferResourcesModified();
        }
        markTopologyModified();
    }

    void markPassModified()
    {
        setDirty(DirtyFlagBits::PassDirty | DirtyFlagBits::TopologyDirty);
    }

    void markImageResourcesModified()
    {
        setDirty(DirtyFlagBits::ImageResourceDirty);
    }

    void markBufferResourcesModified()
    {
        setDirty(DirtyFlagBits::BufferResourceDirty);
    }

    void markBackBufferModified()
    {
        setDirty(DirtyFlagBits::BackBufferDirty);
    }

    void markTopologyModified()
    {
        setDirty(DirtyFlagBits::TopologyDirty);
    }

private:
    vk::Device* m_pDevice = {}; // Will be nullptr in dry run mode
    vk::CommandBufferAllocator* m_pCommandBufferAllocator = {};

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

        HashMap<RenderPass*, vk::CommandBuffer*> cmds;
        HashMap<RenderPass*, SmallVector<vk::ImageBarrier>> imageBarriers;
        HashMap<RenderPass*, SmallVector<vk::BufferBarrier>> bufferBarriers;

        HashMap<PassResource*, vk::Image*> image;
        HashMap<PassResource*, vk::Buffer*> buffer;

        // Resource state tracking at graph level
        HashMap<PassResource*, ResourceState> currentResourceStates;

        vk::SwapChain* pSwapchain = {};
        vk::Fence* frameExecuteFence = {};

        SmallVector<vk::QueueSubmitInfo> frameSubmitInfos{};
        std::mutex submitLock;
    } m_buildData;

    struct
    {
        ThreadSafeObjectPool<PassBufferResource> passBufferResource;
        ThreadSafeObjectPool<PassImageResource> passImageResource;
        ThreadSafeObjectPool<RenderPass> renderPass;
    } m_resourcePool;

    // Debug output for dry run mode
    bool m_debugOutputEnabled = false;
    bool m_forceDryRun = false;

    struct TransientResourceInfo
    {
        uint32_t firstUsePassIndex = UINT32_MAX;
        uint32_t lastUsePassIndex = 0;
        size_t size = 0;
        bool isImage = false;
    };

    HashMap<PassResource*, TransientResourceInfo> m_transientResources;

    void analyzeResourceLifetimes();
    bool isResourceTransient(PassResource* resource) const;

    DebugCaptureInfo m_debugCapture;
    void capturePassOutput(RenderPass* pass, vk::CommandBuffer* cmd);
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

    RDG_LOG_ERR("Could not find the pass resource [%s].", name.c_str());
    APH_ASSERT(false);
    return nullptr;
}
} // namespace aph
