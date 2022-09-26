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

using ResourceIndex    = uint32_t;
using SubEntityList    = std::vector<std::shared_ptr<SubEntity>>;
using PrimitiveList    = std::vector<Primitive>;
using TextureData      = std::vector<unsigned char>;
using VertexLayoutList = std::vector<VertexLayout>;
using IndexList        = std::vector<uint32_t>;
using ImageList        = std::vector<Texture>;
using MaterialList     = std::vector<Material>;

struct VertexLayout {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};

struct Primitive {
    ResourceIndex firstIndex;
    ResourceIndex indexCount;
    ResourceIndex materialIndex;
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
    uint32_t    width;
    uint32_t    height;
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

    ResourceIndex baseColorTextureIndex = 0;
    ResourceIndex diffuseTextureIndex   = 0;
    ResourceIndex specularTextureIndex  = 0;

    ResourceIndex normalTextureIndex    = 0;
    ResourceIndex occlusionTextureIndex = 0;
    ResourceIndex emissiveTextureIndex  = 0;

    ResourceIndex metallicRoughnessTextureIndex = 0;

    ResourceIndex specularGlossinessTextureIndex = 0;
};

class Entity : public Object {
public:
    Entity(SceneManager *manager, IdType id);
    ~Entity() override;
    void loadFromFile(const std::string &path);

public:
    VertexLayoutList _vertices;
    IndexList        _indices;
    ImageList        _images;
    SubEntityList    _subEntityList;
    MaterialList     _materials;
};
} // namespace vkl

#endif // VKLENTITY_H_
