#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "object.h"

namespace vkl {
struct Subset;
struct Texture;
struct Material;
struct Vertex;
struct Node;

using ResourceIndex = int32_t;
using SubNodeList   = std::vector<std::shared_ptr<Node>>;
using SubsetList    = std::vector<Subset>;
using TextureData   = std::vector<unsigned char>;
using VertexList    = std::vector<Vertex>;
using IndexList     = std::vector<uint32_t>;
using TextureList   = std::vector<Texture>;
using MaterialList  = std::vector<Material>;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};

struct Subset {
    ResourceIndex firstIndex    = -1;
    ResourceIndex indexCount    = -1;
    ResourceIndex materialIndex = -1;
};

struct Node {
    std::string name;
    glm::mat4   matrix    = glm::mat4(1.0f);
    bool        isVisible = true;

    Node       *parent;
    SubNodeList children;
    SubsetList  subsets;
};

struct Texture {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;

    TextureData data;
};

struct Material {
    uint32_t id;
    bool     doubleSided = false;
    enum AlphaMode { ALPHAMODE_OPAQUE,
                     ALPHAMODE_MASK,
                     ALPHAMODE_BLEND };
    AlphaMode alphaMode       = ALPHAMODE_OPAQUE;
    float     alphaCutoff     = 1.0f;
    float     metallicFactor  = 1.0f;
    float     roughnessFactor = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

    ResourceIndex baseColorTextureIndex = -1;
    ResourceIndex diffuseTextureIndex   = -1;
    ResourceIndex specularTextureIndex  = -1;

    ResourceIndex normalTextureIndex    = -1;
    ResourceIndex occlusionTextureIndex = -1;
    ResourceIndex emissiveTextureIndex  = -1;

    ResourceIndex metallicRoughnessTextureIndex = -1;

    ResourceIndex specularGlossinessTextureIndex = -1;
};

class Entity : public Object {
    friend class GLTFLoader;
    friend class VulkanRenderData;

public:
    static std::shared_ptr<Entity> Create();
    Entity(IdType id);
    ~Entity() override;
    void loadFromFile(const std::string &path);
    void cleanupResources();

private:
    VertexList   _vertices;
    IndexList    _indices;
    TextureList  _images;
    SubNodeList  _subNodeList;
    MaterialList _materials;

    bool isLoaded = false;
};
} // namespace vkl

#endif // VKLENTITY_H_
