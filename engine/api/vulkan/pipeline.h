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

    Device*        m_pDevice  = {};
    ShaderProgram* m_pProgram = {};
    PipelineType   m_type     = {};
};

class PipelineAllocator
{
public:
    PipelineAllocator(Device* pDevice) : m_pDevice(pDevice) {}
    ~PipelineAllocator() = default;

    Pipeline* getPipeline(const GraphicsPipelineCreateInfo& createInfo);
    Pipeline* getPipeline(const ComputePipelineCreateInfo& createInfo);

    void clear();

private:
    Device*                        m_pDevice = {};
    ThreadSafeObjectPool<Pipeline> m_pool;
    std::mutex                     m_graphicsAcquireLock;
    std::mutex                     m_computeAcquireLock;

private:
    struct PipelineBinaryKeyHash
    {
        std::size_t operator()(const VkPipelineBinaryKeyKHR& keyObj) const noexcept
        {
            using namespace aph::utils;
            std::size_t seed = 0;
            hashCombine(seed, keyObj.keySize);
            const char*      keyData = reinterpret_cast<const char*>(keyObj.key);
            std::string_view keyView{keyData, keyObj.keySize};
            hashCombine(seed, keyView);
            return seed;
        }
    };

    struct PipelineBinaryKeyEqual
    {
        bool operator()(const VkPipelineBinaryKeyKHR& lhs, const VkPipelineBinaryKeyKHR& rhs) const noexcept
        {
            if(lhs.keySize != rhs.keySize)
                return false;
            return std::memcmp(lhs.key, rhs.key, lhs.keySize) == 0;
        }
    };

    void setupPipelineKey(const VkPipelineBinaryKeyKHR& pipelineKey, Pipeline* pPipeline);

    HashMap<VkPipelineBinaryKeyKHR, std::vector<uint8_t>, PipelineBinaryKeyHash, PipelineBinaryKeyEqual>
        m_binaryKeyRawDataMap;
    HashMap<VkPipelineBinaryKeyKHR, VkPipelineBinaryKHR, PipelineBinaryKeyHash, PipelineBinaryKeyEqual>
        m_binaryKeyDataMap;
    HashMap<VkPipelineBinaryKeyKHR, std::vector<VkPipelineBinaryKeyKHR>, PipelineBinaryKeyHash, PipelineBinaryKeyEqual>
        m_pipelineKeyBinaryKeysMap;
    HashMap<VkPipelineBinaryKeyKHR, Pipeline*, PipelineBinaryKeyHash, PipelineBinaryKeyEqual> m_pipelineMap;
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
