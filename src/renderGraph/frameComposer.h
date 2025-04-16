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
    explicit FrameComposer(const FrameComposerCreateInfo& createInfo);
    FrameComposer(const FrameComposer&)                    = delete;
    FrameComposer(FrameComposer&&)                         = delete;
    auto operator=(const FrameComposer&) -> FrameComposer& = delete;
    auto operator=(FrameComposer&&) -> FrameComposer&      = delete;
    // Factory methods
    static auto Create(const FrameComposerCreateInfo& createInfo) -> Expected<FrameComposer*>;
    static void Destroy(FrameComposer* pComposer);

    template <typename T>
    auto getSharedResource(const std::string& resourceName) const -> auto;
    void buildAllGraphs(vk::SwapChain* pSwapChain = nullptr);

    void cleanup();

    struct FrameResource
    {
        RenderGraph* pGraph;
        uint32_t frameIndex;
    };

    auto getFrameCount() const -> uint32_t;
    auto getCurrentGraph() const -> RenderGraph*;
    auto getGraph(uint32_t frameIndex) const -> RenderGraph*;
    auto getCurrentFrame() const -> FrameResource;
    auto nextFrame() -> FrameResource;

    auto frames() -> coro::generator<FrameResource>;

private:
    // Frame management
    void setFrameCount(uint32_t frameCount);
    void setCurrentFrame(uint32_t frameIndex);

private:
    ~FrameComposer();

    auto initialize(const FrameComposerCreateInfo& createInfo) -> Result;
    auto createFrameGraph(uint32_t frameIndex) -> Result;
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
    if constexpr (std::is_same_v<T, ImageAsset>)
    {
        APH_ASSERT(m_buildImage.contains(resourceName));
        return m_buildImage.at(resourceName);
    }
    else if constexpr (std::is_same_v<T, BufferAsset>)
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
