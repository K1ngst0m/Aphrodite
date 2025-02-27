#pragma once

#include "api/gpuResource.h"
#include "allocator/objectPool.h"
#include "common/hash.h"

namespace aph::vk
{
class Device;
class DescriptorSet;
class Sampler;
class ShaderProgram;
class Shader;
struct ImmutableSamplerBank;

struct GraphicsPipelineCreateInfo
{
    PipelineType type = PipelineType::Geometry;

    RenderPipelineDynamicState dynamicState = {};
    PrimitiveTopology          topology     = PrimitiveTopology::TriangleList;

    VertexInput vertexInput;

    ShaderProgram* pProgram = {};

    std::vector<ColorAttachment> color         = {};
    Format                       depthFormat   = Format::Undefined;
    Format                       stencilFormat = Format::Undefined;

    CullMode    cullMode         = CullMode::None;
    WindingMode frontFaceWinding = WindingMode::CCW;
    PolygonMode polygonMode      = PolygonMode::Fill;

    StencilState backFaceStencil  = {};
    StencilState frontFaceStencil = {};

    uint32_t samplesCount = 1u;
};

struct ComputePipelineCreateInfo
{
    ImmutableSamplerBank* pSamplerBank = {};
    ShaderProgram*        pProgram     = {};
};


class Pipeline : public ResourceHandle<VkPipeline>
{
    friend class ObjectPool<Pipeline>;

public:
    ShaderProgram* getProgram() const { return m_pProgram; }
    PipelineType   getType() const { return m_type; }

protected:
    Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, HandleType handle);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle);

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

    struct PipelineBinaryData
    {
        std::vector<uint8_t> rawData;
        VkPipelineBinaryKHR binary;
    };

    template<typename Key, typename Val>
    using PipelineKeyMap = HashMap<Key, Val, PipelineBinaryKeyHash, PipelineBinaryKeyEqual>;

    PipelineKeyMap<VkPipelineBinaryKeyKHR, PipelineBinaryData> m_binaryKeyDataMap;
    PipelineKeyMap<VkPipelineBinaryKeyKHR, std::vector<VkPipelineBinaryKeyKHR>> m_pipelineKeyBinaryKeysMap;
    PipelineKeyMap<VkPipelineBinaryKeyKHR, Pipeline*> m_pipelineMap;
};

}  // namespace aph::vk
