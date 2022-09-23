#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "object.h"

namespace vkl {
class EntityLoader;
class ShaderPass;
struct Primitive;
struct SubMesh;
struct Image;
struct Material;
struct SubEntity;

using ResourceIndex    = uint32_t;
using SubEntityList    = std::vector<SubEntity *>;
using PrimitiveList    = std::vector<Primitive>;
using ImageData        = std::vector<unsigned char>;
using VertexLayoutList = std::vector<VertexLayout>;
using IndexList        = std::vector<uint32_t>;
using ImageList        = std::vector<Image>;
using MaterialList     = std::vector<Material>;

struct Primitive {
    ResourceIndex firstIndex;
    ResourceIndex indexCount;
    ResourceIndex materialIndex;
};
struct SubMesh {
    PrimitiveList primitives;
    void          pushPrimitive(ResourceIndex firstIdx, ResourceIndex indexCount, ResourceIndex materialIdx) {
        primitives.push_back({firstIdx, indexCount, materialIdx});
    }
};
struct SubEntity {
    SubEntity    *parent;
    SubEntityList children;
    SubMesh       mesh;
    glm::mat4     matrix;
    ~SubEntity(){
        if (!children.empty()){
            for (auto * subEntity : children){
                delete subEntity;
            }
        }
    }
};
struct Image {
    uint32_t  width;
    uint32_t  height;
    ImageData data;
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

    ResourceIndex baseColorTextureIndex          = 0;
    ResourceIndex normalTextureIndex             = 0;
    ResourceIndex metallicRoughnessTextureIndex  = 0;
    ResourceIndex occlusionTextureIndex          = 0;
    ResourceIndex emissiveTextureIndex           = 0;
    ResourceIndex specularGlossinessTextureIndex = 0;
    ResourceIndex diffuseTextureIndex            = 0;
    ResourceIndex specularTextureIndex           = 0;
};

class Entity : public Object {
public:
    Entity(SceneManager *manager);
    ~Entity() override;
    void loadFromFile(const std::string &path);

public:
    VertexLayoutList _vertices;
    IndexList        _indices;
    ImageList        _images;
    SubEntityList    _subEntityList;
    MaterialList     _materials;

protected:
    vkl::EntityLoader *_loader;
};

class EntityLoader {
public:
    EntityLoader(Entity *entity);

    virtual void load() = 0;

protected:
    Entity *_entity;
};

} // namespace vkl

#endif // VKLENTITY_H_
