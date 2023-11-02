#ifndef APH_RDG_H_
#define APH_RDG_H_

#include "api/vulkan/device.h"

#include <unordered_set>

namespace aph
{

class RenderGraph;

struct AttachmentInfo
{
    float    sizeX      = 1.0f;
    float    sizeY      = 1.0f;
    VkFormat format     = VK_FORMAT_UNDEFINED;
    uint32_t samples    = 1;
    uint32_t levels     = 1;
    uint32_t layers     = 1;
    bool     persistent = true;
};

struct BufferInfo
{
    std::size_t        size       = 0;
    VkBufferUsageFlags usage      = 0;
    bool               persistent = true;
};

struct ResourceDimensions
{
};

class RenderResource
{
public:
    enum class Type
    {
        Buffer,
        Image,
    };

    RenderResource(Type type, uint32_t index, std::string_view name) :
        m_resourceType(type),
        m_name(name),
        m_index(index)
    {
    }

    virtual ~RenderResource() = default;

    void writtenInPass(uint32_t index) { m_writtenInPasses.insert(index); }
    void readInPass(uint32_t index) { m_readInPasses.insert(index); }

private:
    Type                         m_resourceType;
    std::unordered_set<unsigned> m_writtenInPasses;
    std::unordered_set<unsigned> m_readInPasses;
    std::string                  m_name;
    uint32_t                     m_index;
    QueueType                    m_queueType = QueueType::Graphics;
};

class RenderNode
{
public:
    RenderNode(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name) :
        m_pRenderGraph(pRDG),
        m_index(index),
        m_queueType(queueType),
        m_name(name)
    {
    }

    RenderNode* addBufferInput(const BufferInfo& info, std::string_view name) { return this; }
    RenderNode* addImageInput(const AttachmentInfo& info, std::string_view name) { return this; }
    RenderNode* addAttachmentInput(const AttachmentInfo& info, std::string_view name) { return this; }
    RenderNode* addColorOuput(const AttachmentInfo& info, std::string_view name) { return this; }
    RenderNode* setDepthStencilOutput(const AttachmentInfo& info, std::string_view name) { return this; }
    RenderNode* addStorageOutput(const AttachmentInfo& info, std::string_view name) { return this; }

    void build() {}

public:
    using BuildRenderPassCallBack   = std::function<void(vk::CommandBuffer*)>;
    using ClearDepthStencilCallBack = std::function<bool(VkClearDepthStencilValue*)>;
    using ClearColorCallBack        = std::function<bool(unsigned, VkClearColorValue*)>;

    void setCallback(BuildRenderPassCallBack&& cb) { m_buildRenderPassCB = cb; }
    void setCallback(ClearColorCallBack&& cb) { m_clearColorCB = cb; }
    void setCallback(ClearDepthStencilCallBack&& cb) { m_clearDepthStencilCB = cb; }

private:
    BuildRenderPassCallBack   m_buildRenderPassCB;
    ClearDepthStencilCallBack m_clearDepthStencilCB;
    ClearColorCallBack        m_clearColorCB;

private:
    RenderGraph* m_pRenderGraph = {};
    uint32_t     m_index        = {};
    QueueType    m_queueType    = {};
    std::string  m_name;
};

class RenderGraph : public Singleton<RenderGraph>
{
public:
    RenderGraph(vk::Device* pDevice);

    RenderNode* addPass(std::string_view name, QueueType queueType);
    RenderNode* getPass(std::string_view name);

    void setOutput(uint32_t idx);

    void reset();
    void bake();

private:
    vk::Device* m_pDevice = {};

    std::vector<RenderNode*>     m_passes;
    std::vector<RenderResource*> m_resources;

    struct
    {
        ThreadSafeObjectPool<RenderNode>     renderPass;
        ThreadSafeObjectPool<RenderResource> renderResource;
    } m_resourcePool;
};

}  // namespace aph

#endif
