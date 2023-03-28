#ifndef MESH_H_
#define MESH_H_

#include "object.h"

namespace vkl
{
struct SceneNode;
struct ImageInfo;
struct Material;
struct Vertex;

using ResourceIndex = int32_t;
using ImageData = std::vector<uint8_t>;
using ImageList = std::vector<std::shared_ptr<ImageInfo>>;
using MaterialList = std::vector<std::shared_ptr<Material>>;

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

    ImageData data;
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
    ResourceIndex baseColorTextureIndex{ -1 };
    ResourceIndex normalTextureIndex{ -1 };
    ResourceIndex occlusionTextureIndex{ -1 };
    ResourceIndex emissiveTextureIndex{ -1 };
    ResourceIndex metallicRoughnessTextureIndex{ -1 };
    ResourceIndex specularGlossinessTextureIndex{ -1 };

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
    std::vector<Vertex> m_vertices{};
    std::vector<uint8_t> m_indices{};
    std::vector<Subset> m_subsets{};
    IndexType m_indexType{ IndexType::UINT32 };
    PrimitiveTopology m_topology{ PrimitiveTopology::TRI_LIST };
};
}  // namespace vkl

#endif  // MESH_H_
