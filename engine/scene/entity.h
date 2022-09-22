#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "api/vulkan/pipeline.h"
#include "object.h"

namespace vkl {
class EntityLoader;

using ResourceIndex = uint32_t;

struct Primitive {
    ResourceIndex firstIndex;
    ResourceIndex indexCount;
    ResourceIndex materialIndex;
};
struct SubMesh {
    std::vector<Primitive> primitives;
    void                   pushPrimitive(ResourceIndex firstIdx, ResourceIndex indexCount, ResourceIndex materialIdx) {
        primitives.push_back({firstIdx, indexCount, materialIdx});
    }
};
struct SubEntity {
    SubEntity               *parent;
    std::vector<SubEntity *> children;
    SubMesh                  mesh;
    glm::mat4                matrix;
};
struct Image {
    uint32_t       width;
    uint32_t       height;
    unsigned char *data;
    size_t         dataSize;
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
    void setShaderPass(vkl::ShaderPass *pass);

public:
    std::vector<VertexLayout> _vertices;
    std::vector<uint32_t>     _indices;
    std::vector<Image *>      _images;
    std::vector<SubEntity *>  _subEntityList;
    std::vector<Material>     _materials;

public:
    vkl::ShaderPass *getPass() {
        return _pass;
    }

protected:
    vkl::ShaderPass   *_pass = nullptr;
    vkl::EntityLoader *_loader;
};

class EntityLoader {
public:
    EntityLoader(Entity *entity);

    virtual void loadFromFile(const std::string &path) = 0;

protected:
    Entity *_entity;
};

} // namespace vkl

#endif // VKLENTITY_H_
