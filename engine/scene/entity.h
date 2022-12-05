#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include <utility>

#include "object.h"

namespace vkl
{
struct Subset;
struct ImageDesc;
struct Material;
struct Vertex;

using ResourceIndex = int32_t;
using SubsetList = std::vector<Subset>;
using ImageData = std::vector<uint8_t>;
using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint32_t>;
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

struct Subset
{
    ResourceIndex firstIndex = -1;
    ResourceIndex indexCount = -1;
    ResourceIndex materialIndex = -1;
};

struct MeshNode : Node<MeshNode>
{
    MeshNode(std::shared_ptr<MeshNode> parent, glm::mat4 matrix = glm::mat4(1.0f)) : Node<MeshNode>(std::move(parent), matrix) {}
    bool isVisible = true;
    SubsetList subsets;
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
    float alphaCutoff = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec4 emissiveFactor = glm::vec4(1.0f);
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

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

class Entity : public Object
{
public:
    static std::shared_ptr<Entity> Create();
    Entity(IdType id) : Object(id) { m_rootNode = std::make_shared<MeshNode>(nullptr); }
    ~Entity() override = default;
    void loadFromFile(const std::string &path);
    void cleanupResources();

    std::shared_ptr<MeshNode> m_rootNode;

    VertexList m_vertices;
    IndexList m_indices;
    TextureList m_images;
    MaterialList m_materials;
};
}  // namespace vkl

#endif  // VKLENTITY_H_
