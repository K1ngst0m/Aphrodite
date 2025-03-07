#pragma once

#include "api/gpuResource.h"
#include "math/math.h"
#include "object.h"

namespace aph
{
using ResourceIndex = int32_t;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec3 tangent;
};

enum class AlphaMode : uint32_t
{
    Opaque = 0,
    Mask = 1,
    Blend = 2
};

struct Material
{
    glm::vec4 emissiveFactor{ 1.0f };
    glm::vec4 baseColorFactor{ 1.0f };

    float alphaCutoff{ 1.0f };
    float metallicFactor{ 1.0f };
    float roughnessFactor{ 1.0f };

    ResourceIndex baseColorId{ -1 };
    ResourceIndex normalId{ -1 };
    ResourceIndex occlusionId{ -1 };
    ResourceIndex emissiveId{ -1 };
    ResourceIndex metallicRoughnessId{ -1 };
    ResourceIndex specularGlossinessId{ -1 };

    bool doubleSided{ false };
    AlphaMode alphaMode{ AlphaMode::Opaque };
    uint32_t id{ 0 };
};

struct Mesh : public Object<Mesh>
{
    Mesh()
        : Object(ObjectType::Mesh)
    {
    }
    struct Subset
    {
        ResourceIndex firstIndex{ -1 };
        ResourceIndex firstVertex{ -1 };
        ResourceIndex vertexCount{ -1 };
        ResourceIndex indexCount = { -1 };
        ResourceIndex materialIndex{ -1 };
        bool hasIndices{ false };
    };
    ResourceIndex m_indexOffset{ -1 };
    ResourceIndex m_vertexOffset{ -1 };
    std::vector<Subset> m_subsets{};
    IndexType m_indexType{ IndexType::UINT32 };
};
} // namespace aph
