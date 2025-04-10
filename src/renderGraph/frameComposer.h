#pragma once

#include "api/vulkan/device.h"
#include "common/result.h"
#include "renderGraph.h"
#include "resource/resourceLoader.h"

namespace aph
{

// Forward declarations
class ResourceLoader;

struct FrameComposerCreateInfo
{
    vk::Device* pDevice             = nullptr;
    ResourceLoader* pResourceLoader = nullptr;
    uint32_t frameCount             = 1;
};

// Class to manage multiple RenderGraph instances with shared resources
class FrameComposer
{
public:
    // Factory methods
    static Expected<FrameComposer*> Create(const FrameComposerCreateInfo& createInfo);
    static void Destroy(FrameComposer* pComposer);

    template <typename T>
    auto getSharedResource(const std::string& resourceName) const;
    void buildAllGraphs(vk::SwapChain* pSwapChain = nullptr);

    void cleanup();

    struct FrameResource
    {
        RenderGraph* pGraph;
        uint32_t frameIndex;
    };

    uint32_t getFrameCount() const;
    RenderGraph* getCurrentGraph() const;
    RenderGraph* getGraph(uint32_t frameIndex) const;
    FrameResource getCurrentFrame() const;
    FrameResource nextFrame();

    coro::generator<FrameResource> frames();

private:
    // Frame management
    void setFrameCount(uint32_t frameCount);
    void setCurrentFrame(uint32_t frameIndex);

private:
    FrameComposer(const FrameComposerCreateInfo& createInfo);
    FrameComposer(const FrameComposer&)            = delete;
    FrameComposer(FrameComposer&&)                 = delete;
    FrameComposer& operator=(const FrameComposer&) = delete;
    FrameComposer& operator=(FrameComposer&&)      = delete;
    ~FrameComposer();

    Result initialize(const FrameComposerCreateInfo& createInfo);
    Result createFrameGraph(uint32_t frameIndex);
    void syncSharedResources();

private:
    vk::Device* m_pDevice             = nullptr;
    ResourceLoader* m_pResourceLoader = nullptr;
    SmallVector<RenderGraph*> m_frameGraphs;

    HashMap<std::string, ImageAsset*> m_buildImage;
    HashMap<std::string, BufferAsset*> m_buildBuffer;
    HashMap<std::string, ShaderAsset*> m_buildShader;

    // Current frame tracking
    uint32_t m_frameCount   = 1;
    uint32_t m_currentFrame = 0;
};

// Implementation of template methods
template <typename T>
auto FrameComposer::getSharedResource(const std::string& resourceName) const
{
    if constexpr (std::is_same_v<T, vk::Image>)
    {
        APH_ASSERT(m_buildImage.contains(resourceName));
        return m_buildImage.at(resourceName);
    }
    else if constexpr (std::is_same_v<T, vk::Buffer>)
    {
        APH_ASSERT(m_buildBuffer.contains(resourceName));
        return m_buildBuffer.at(resourceName);
    }
    else if constexpr (std::is_same_v<T, ShaderAsset>)
    {
        APH_ASSERT(m_buildShader.contains(resourceName));
        return m_buildShader.at(resourceName);
    }
    else
    {
        static_assert(dependent_false_v<T>, "unsupported resource type.");
        return BufferAsset{};
    }
}
} // namespace aph
