#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "api/gpuResource.h"
#include "allocator/objectPool.h"
#include "common/hash.h"
#include "volk.h"

namespace aph::vk
{
class Device;
class DescriptorSet;
class Sampler;
class ShaderProgram;
class Shader;
struct ImmutableSamplerBank;

struct ColorAttachment
{
    Format      format              = Format::Undefined;
    bool        blendEnabled        = false;
    BlendOp     rgbBlendOp          = BlendOp::Add;
    BlendOp     alphaBlendOp        = BlendOp::Add;
    BlendFactor srcRGBBlendFactor   = BlendFactor::One;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstRGBBlendFactor   = BlendFactor::Zero;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;

    bool operator==(const ColorAttachment& rhs) const
    {
        return format == rhs.format && blendEnabled == rhs.blendEnabled && rgbBlendOp == rhs.rgbBlendOp &&
               alphaBlendOp == rhs.alphaBlendOp && srcRGBBlendFactor == rhs.srcRGBBlendFactor &&
               srcAlphaBlendFactor == rhs.srcAlphaBlendFactor && dstRGBBlendFactor == rhs.dstAlphaBlendFactor &&
               dstAlphaBlendFactor == rhs.dstAlphaBlendFactor;
    }
};

struct StencilState
{
    StencilOp stencilFailureOp   = StencilOp::Keep;
    StencilOp depthFailureOp     = StencilOp::Keep;
    StencilOp depthStencilPassOp = StencilOp::Keep;
    CompareOp stencilCompareOp   = CompareOp::Always;
    uint32_t  readMask           = (uint32_t)~0;
    uint32_t  writeMask          = (uint32_t)~0;

    bool operator==(const StencilState& rhs) const
    {
        return stencilFailureOp == rhs.stencilFailureOp && stencilCompareOp == rhs.stencilCompareOp &&
               depthStencilPassOp == rhs.depthStencilPassOp && depthFailureOp == rhs.depthFailureOp &&
               readMask == rhs.readMask && writeMask == rhs.writeMask;
    }
};

struct RenderPipelineDynamicState final
{
    bool depthBiasEnable = false;
    bool operator==(const RenderPipelineDynamicState& rhs) const { return depthBiasEnable == rhs.depthBiasEnable; }
};

struct GraphicsPipelineCreateInfo
{
    PipelineType type = PipelineType::Geometry;

    RenderPipelineDynamicState dynamicState = {};
    PrimitiveTopology          topology     = PrimitiveTopology::TriangleList;

    VertexInput vertexInput;

    ShaderProgram* pProgram = {};

    ImmutableSamplerBank* pSamplerBank = {};

    std::vector<ColorAttachment> color         = {};
    Format                       depthFormat   = Format::Undefined;
    Format                       stencilFormat = Format::Undefined;

    CullMode    cullMode         = CullMode::None;
    WindingMode frontFaceWinding = WindingMode::CCW;
    PolygonMode polygonMode      = PolygonMode::Fill;

    StencilState backFaceStencil  = {};
    StencilState frontFaceStencil = {};

    uint32_t samplesCount = 1u;

    bool operator==(const GraphicsPipelineCreateInfo& rhs) const
    {
        return dynamicState == rhs.dynamicState && topology == rhs.topology && vertexInput == rhs.vertexInput &&
               pProgram == rhs.pProgram && pSamplerBank == rhs.pSamplerBank && depthFormat == rhs.depthFormat &&
               stencilFormat == rhs.stencilFormat && polygonMode == rhs.polygonMode &&
               backFaceStencil == rhs.backFaceStencil && frontFaceStencil == rhs.frontFaceStencil &&
               frontFaceWinding == rhs.frontFaceWinding && samplesCount == rhs.samplesCount;
    }
};

struct ComputePipelineCreateInfo
{
    ImmutableSamplerBank* pSamplerBank = {};
    ShaderProgram*        pCompute     = {};

    bool operator==(const ComputePipelineCreateInfo& rhs) const
    {
        return pSamplerBank == rhs.pSamplerBank && pCompute == rhs.pCompute;
    }
};

class Pipeline : public ResourceHandle<VkPipeline>
{
    friend class ObjectPool<Pipeline>;

public:
    DescriptorSet* acquireSet(uint32_t idx) const;

    ShaderProgram* getProgram() const { return m_pProgram; }
    PipelineType   getType() const { return m_type; }

protected:
    Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, HandleType handle, ShaderProgram* pProgram);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle, ShaderProgram* pProgram);

    Device*         m_pDevice  = {};
    ShaderProgram*  m_pProgram = {};
    PipelineType    m_type     = {};
};

class PipelineAllocator
{
    struct HashGraphicsPipeline
    {
        std::size_t operator()(const ComputePipelineCreateInfo& info) const noexcept
        {
            std::size_t seed = 0;
            aph::utils::hashCombine(seed, info.pCompute);
            aph::utils::hashCombine(seed, info.pSamplerBank);
            return seed;
        }

        std::size_t operator()(const GraphicsPipelineCreateInfo& info) const noexcept
        {
            std::size_t seed = 0;
            aph::utils::hashCombine(seed, info.dynamicState.depthBiasEnable);
            aph::utils::hashCombine(seed, info.topology);

            {
                for(const auto& attr : info.vertexInput.attributes)
                {
                    aph::utils::hashCombine(seed, attr.binding);
                    aph::utils::hashCombine(seed, attr.format);
                    aph::utils::hashCombine(seed, attr.location);
                    aph::utils::hashCombine(seed, attr.offset);
                }
                for(const auto& binding : info.vertexInput.bindings)
                {
                    aph::utils::hashCombine(seed, binding.stride);
                }
            }

            aph::utils::hashCombine(seed, info.pProgram);

            for(auto color : info.color)
            {
                aph::utils::hashCombine(seed, color.format);
                aph::utils::hashCombine(seed, color.blendEnabled);
                aph::utils::hashCombine(seed, color.rgbBlendOp);
                aph::utils::hashCombine(seed, color.alphaBlendOp);
                aph::utils::hashCombine(seed, color.srcRGBBlendFactor);
                aph::utils::hashCombine(seed, color.srcAlphaBlendFactor);
                aph::utils::hashCombine(seed, color.dstRGBBlendFactor);
                aph::utils::hashCombine(seed, color.dstAlphaBlendFactor);
            }

            aph::utils::hashCombine(seed, info.depthFormat);
            aph::utils::hashCombine(seed, info.stencilFormat);

            aph::utils::hashCombine(seed, info.cullMode);
            aph::utils::hashCombine(seed, info.frontFaceWinding);
            aph::utils::hashCombine(seed, info.polygonMode);

            {
                auto& stencilState = info.backFaceStencil;
                aph::utils::hashCombine(seed, stencilState.stencilFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthStencilPassOp);
                aph::utils::hashCombine(seed, stencilState.stencilCompareOp);
                aph::utils::hashCombine(seed, stencilState.readMask);
                aph::utils::hashCombine(seed, stencilState.writeMask);
            }

            {
                auto& stencilState = info.frontFaceStencil;
                aph::utils::hashCombine(seed, stencilState.stencilFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthStencilPassOp);
                aph::utils::hashCombine(seed, stencilState.stencilCompareOp);
                aph::utils::hashCombine(seed, stencilState.readMask);
                aph::utils::hashCombine(seed, stencilState.writeMask);
            }

            aph::utils::hashCombine(seed, info.samplesCount);

            return seed;
        }
    };

public:
    PipelineAllocator(Device* pDevice) : m_pDevice(pDevice) {}
    ~PipelineAllocator();

    void clear();

    Pipeline* getPipeline(const GraphicsPipelineCreateInfo& createInfo);
    Pipeline* getPipeline(const ComputePipelineCreateInfo& createInfo);

private:
    Pipeline* create(const GraphicsPipelineCreateInfo& createInfo);
    Pipeline* create(const ComputePipelineCreateInfo& createInfo);

    Device*                                                              m_pDevice = {};
    HashMap<GraphicsPipelineCreateInfo, Pipeline*, HashGraphicsPipeline> m_graphicsPipelineMap;
    HashMap<ComputePipelineCreateInfo, Pipeline*, HashGraphicsPipeline>  m_computePipelineMap;
    ThreadSafeObjectPool<Pipeline>                                       m_pool;
    std::mutex                                                           m_graphicsAcquireLock;
    std::mutex                                                           m_computeAcquireLock;
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
