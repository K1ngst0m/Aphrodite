#ifndef MESH_H_
#define MESH_H_

#include "object.h"

namespace aph
{
struct SceneNode;
struct ImageInfo;
struct Material;
struct Vertex;

using ResourceIndex = int32_t;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};

struct ImageInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;

    std::vector<uint8_t> data;
};

enum class AlphaMode : uint32_t
{
    OPAQUE = 0,
    MASK = 1,
    BLEND = 2
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
    AlphaMode alphaMode{ AlphaMode::OPAQUE };
    uint32_t id{ 0 };
};

enum class IndexType
{
    UINT16,
    UINT32,
};

enum class PrimitiveTopology
{
    TRI_LIST,
    TRI_STRIP,
};

struct Mesh : public Object
{
    Mesh() : Object(Id::generateNewId<Mesh>(), ObjectType::MESH) {}
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
    PrimitiveTopology m_topology{ PrimitiveTopology::TRI_LIST };
};
}  // namespace aph

#endif  // MESH_H_
