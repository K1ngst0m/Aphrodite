#ifndef ENTITY_H_
#define ENTITY_H_

#include <utility>

#include "node.h"
#include "object.h"

namespace vkl
{
struct SceneNode;
struct ImageDesc;
struct Material;
struct Vertex;

using ResourceIndex = int32_t;
using ImageData = std::vector<uint8_t>;
using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint8_t>;
using TextureList = std::vector<std::shared_ptr<ImageDesc>>;
using MaterialList = std::vector<std::shared_ptr<Material>>;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};

struct ImageDesc
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
    glm::vec4 emissiveFactor = glm::vec4(1.0f);
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float alphaCutoff = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    ResourceIndex baseColorTextureIndex = -1;
    ResourceIndex normalTextureIndex = -1;
    ResourceIndex occlusionTextureIndex = -1;
    ResourceIndex emissiveTextureIndex = -1;
    ResourceIndex metallicRoughnessTextureIndex = -1;
    ResourceIndex specularGlossinessTextureIndex = -1;

    bool doubleSided = false;
    AlphaMode alphaMode = AlphaMode::OPAQUE;
    uint32_t id;
};

enum class IndexType{
    UINT16,
    UINT32,
};

struct Mesh : public Object
{
    Mesh() : Object(Id::generateNewId<Mesh>(), ObjectType::MESH) {}
    struct Subset
    {
        ResourceIndex firstIndex = -1;
        ResourceIndex firstVertex = -1;
        ResourceIndex vertexCount = -1;
        ResourceIndex indexCount = -1;
        ResourceIndex materialIndex = -1;
        bool hasIndices = false;
    };
    std::vector<Subset> m_subsets;
    IndexType m_indexType = IndexType::UINT32;
};

class Entity : public Object
{
public:
    Entity() :
        Object(Id::generateNewId<Entity>(), ObjectType::ENTITY),
        m_rootNode(std::make_shared<SceneNode>(nullptr))
    {
    }
    ~Entity() override;
    void loadFromFile(const std::string &path);

    std::shared_ptr<SceneNode> m_rootNode = nullptr;

    VertexList m_vertices;
    IndexList m_indices;
    TextureList m_images;
    MaterialList m_materials;
};
}  // namespace vkl

#endif  // VKLENTITY_H_
