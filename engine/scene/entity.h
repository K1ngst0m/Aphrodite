#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "object.h"

namespace vkl {
class EntityLoader;
class ShaderPass;
struct Primitive;
struct SubMesh;
struct Texture;
struct Material;
struct SubEntity;
struct VertexLayout;

using ResourceIndex    = int32_t;
using SubEntityList    = std::vector<std::shared_ptr<SubEntity>>;
using PrimitiveList    = std::vector<Primitive>;
using TextureData      = std::vector<unsigned char>;
using VertexLayoutList = std::vector<VertexLayout>;
using IndexList        = std::vector<uint32_t>;
using TextureList      = std::vector<Texture>;
using MaterialList     = std::vector<Material>;

struct VertexLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};

struct Primitive {
    ResourceIndex firstIndex    = -1;
    ResourceIndex indexCount    = -1;
    ResourceIndex materialIndex = -1;
};

struct SubEntity {
    std::string   name;
    bool          isVisible = true;
    SubEntity    *parent;
    SubEntityList children;
    PrimitiveList primitives;
    glm::mat4     matrix;
};

struct Texture {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;

    TextureData data;
};

struct Material {
    bool doubleSided = false;
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
public:
    Entity(SceneManager *manager, IdType id);
    ~Entity() override;
    void loadFromFile(const std::string &path);

public:
    VertexLayoutList _vertices;
    IndexList        _indices;
    TextureList      _textures;
    SubEntityList    _subEntityList;
    MaterialList     _materials;

private:
    bool isLoaded = false;
    void cleanupResources();
};
} // namespace vkl

#endif // VKLENTITY_H_
