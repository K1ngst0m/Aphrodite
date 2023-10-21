#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class DescriptorSet;
class Sampler;
class ShaderProgram;
class Shader;
struct ImmutableSamplerBank;

enum
{
    APH_MAX_COLOR_ATTACHMENTS = 5
};
enum
{
    APH_MAX_MIP_LEVELS = 16
};

struct ColorAttachment
{
    Format        format              = Format::Undefined;
    bool          blendEnabled        = false;
    VkBlendOp     rgbBlendOp          = VK_BLEND_OP_ADD;
    VkBlendOp     alphaBlendOp        = VK_BLEND_OP_ADD;
    VkBlendFactor srcRGBBlendFactor   = VK_BLEND_FACTOR_ONE;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstRGBBlendFactor   = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
};

struct StencilState
{
    VkStencilOp stencilFailureOp   = VK_STENCIL_OP_KEEP;
    VkStencilOp depthFailureOp     = VK_STENCIL_OP_KEEP;
    VkStencilOp depthStencilPassOp = VK_STENCIL_OP_KEEP;
    VkCompareOp stencilCompareOp   = VK_COMPARE_OP_ALWAYS;
    uint32_t    readMask           = (uint32_t)~0;
    uint32_t    writeMask          = (uint32_t)~0;
};

struct RenderPipelineDynamicState final
{
    VkBool32 depthBiasEnable = VK_FALSE;
};

struct GraphicsPipelineCreateInfo
{
    RenderPipelineDynamicState dynamicState = {};
    VkPrimitiveTopology        topology     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VertexInput vertexInput;

    Shader* pVertex   = {};
    Shader* pFragment = {};

    ImmutableSamplerBank* pSamplerBank = {};

    ColorAttachment color[APH_MAX_COLOR_ATTACHMENTS] = {};
    VkFormat        depthFormat                      = VK_FORMAT_UNDEFINED;
    VkFormat        stencilFormat                    = VK_FORMAT_UNDEFINED;

    VkCullModeFlags cullMode         = VK_CULL_MODE_NONE;
    VkFrontFace     frontFaceWinding = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPolygonMode   polygonMode      = VK_POLYGON_MODE_FILL;

    StencilState backFaceStencil  = {};
    StencilState frontFaceStencil = {};

    uint32_t samplesCount = 1u;

    const char* debugName = "";

    uint32_t getNumColorAttachments() const
    {
        uint32_t n = 0;
        while(n < APH_MAX_COLOR_ATTACHMENTS && color[n].format != Format::Undefined)
        {
            n++;
        }
        return n;
    }
};

struct RenderPipelineState
{
    GraphicsPipelineCreateInfo createInfo;

    std::vector<VkVertexInputBindingDescription>   vkBindings    = {};
    std::vector<VkVertexInputAttributeDescription> vkAttributes  = {};

    // non-owning, cached the last pipeline layout from the context (if the context has a new layout, invalidate all
    // VkPipeline objects)
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // [depthBiasEnable]
    VkPipeline pipelines[2] = {};
};

struct ComputePipelineCreateInfo
{
    ImmutableSamplerBank* pSamplerBank = {};
    Shader*               pCompute     = {};
};

class VulkanPipelineBuilder final
{
public:
    VulkanPipelineBuilder();
    ~VulkanPipelineBuilder() = default;

    VulkanPipelineBuilder& depthBiasEnable(bool enable);
    VulkanPipelineBuilder& dynamicState(VkDynamicState state);
    VulkanPipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);
    VulkanPipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples);
    VulkanPipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);
    VulkanPipelineBuilder& shaderStage(const std::vector<VkPipelineShaderStageCreateInfo>& stages);
    VulkanPipelineBuilder& stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp,
                                           VkStencilOp depthFailOp, VkCompareOp compareOp);
    VulkanPipelineBuilder& stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask,
                                        uint32_t reference);
    VulkanPipelineBuilder& cullMode(VkCullModeFlags mode);
    VulkanPipelineBuilder& frontFace(VkFrontFace mode);
    VulkanPipelineBuilder& polygonMode(VkPolygonMode mode);
    VulkanPipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
    VulkanPipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats,
                                            uint32_t numColorAttachments);
    VulkanPipelineBuilder& depthAttachmentFormat(VkFormat format);
    VulkanPipelineBuilder& stencilAttachmentFormat(VkFormat format);

    VkResult build(Device* pDevice, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout,
                   VkPipeline* outPipeline, const char* debugName = nullptr) noexcept;

    static uint32_t getNumPipelinesCreated() { return numPipelinesCreated_; }

private:
    enum
    {
        APH_MAX_DYNAMIC_STATES = 128
    };
    uint32_t       numDynamicStates_                      = 0;
    VkDynamicState dynamicStates_[APH_MAX_DYNAMIC_STATES] = {};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages_ = {};

    VkPipelineVertexInputStateCreateInfo   vertexInputState_;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
    VkPipelineRasterizationStateCreateInfo rasterizationState_;
    VkPipelineMultisampleStateCreateInfo   multisampleState_;
    VkPipelineDepthStencilStateCreateInfo  depthStencilState_;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates_ = {};
    std::vector<VkFormat>                            colorAttachmentFormats_     = {};

    VkFormat depthAttachmentFormat_   = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

    static uint32_t numPipelinesCreated_;
};

class Pipeline : public ResourceHandle<VkPipeline>
{
public:
    Pipeline(Device* pDevice, const RenderPipelineState& rps, HandleType handle, ShaderProgram* pProgram);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle, ShaderProgram* pProgram);

    DescriptorSet* acquireSet(uint32_t idx) const;

    ShaderProgram*      getProgram() const { return m_pProgram; }
    VkPipelineBindPoint getBindPoint() const { return m_bindPoint; }

protected:
    Device*             m_pDevice   = {};
    ShaderProgram*      m_pProgram  = {};
    VkPipelineBindPoint m_bindPoint = {};
    VkPipelineCache     m_cache     = {};
    RenderPipelineState m_rps       = {};
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
