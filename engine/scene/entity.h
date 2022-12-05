#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include <utility>

#include "object.h"

namespace vkl
{
struct Subset;
struct Image;
struct Material;
struct Vertex;
struct Node;

using ResourceIndex = int32_t;
using SubNodeList = std::vector<std::shared_ptr<Node>>;
using SubsetList = std::vector<Subset>;
using ImageData = std::vector<uint8_t>;
using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint32_t>;
using TextureList = std::vector<std::shared_ptr<Image>>;
using MaterialList = std::vector<Material>;

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

struct Node : std::enable_shared_from_this<Node>
{
    Node(std::shared_ptr<Node> parent) : parent(std::move(parent)) {}
    std::shared_ptr<Node> createChildNode();
    std::string name;
    glm::mat4 matrix = glm::mat4(1.0f);
    bool isVisible = true;

    std::shared_ptr<Node> parent;
    SubNodeList children;
    SubsetList subsets;
};

struct Image
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
    Entity(IdType id) : Object(id) { m_rootNode = std::make_shared<Node>(nullptr); }
    ~Entity() override = default;
    void loadFromFile(const std::string &path);
    void cleanupResources();

    std::shared_ptr<Node> m_rootNode;

    VertexList _vertices;
    IndexList _indices;
    TextureList _images;
    MaterialList _materials;
};
}  // namespace vkl

#endif  // VKLENTITY_H_
