#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "api/vulkan/pipeline.h"
#include "object.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vklt {
    class EntityLoader;
}

namespace vkl {

using TextureIndex = uint32_t;
class Entity : public Object {
public:
    Entity(SceneManager *manager);
    ~Entity() override;
    void loadFromFile(const std::string &path);
    void setShaderPass(vkl::ShaderPass *pass);

public:
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t  materialIndex;
    };
    struct Mesh {
        std::vector<Primitive> primitives;
        void pushPrimitive(uint32_t firstIdx, uint32_t indexCount, int32_t materialIdx) {
            primitives.push_back({firstIdx, indexCount, materialIdx});
        }
    };
    struct Node {
        Node               *parent;
        std::vector<Node *> children;
        Mesh                mesh;
        glm::mat4           matrix;
    };
    struct Image {
        uint32_t       width;
        uint32_t       height;
        unsigned char *data;
        size_t         dataSize;
    };
    struct Material {
        bool doubleSided = false;
        enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor       = glm::vec4(1.0f);

        TextureIndex baseColorTextureIndex  = 0;
        TextureIndex normalTextureIndex = 0;
        TextureIndex metallicRoughnessTextureIndex = 0;
        TextureIndex occlusionTextureIndex = 0;
        TextureIndex emissiveTextureIndex = 0;
        TextureIndex specularGlossinessTextureIndex = 0;
        TextureIndex diffuseTextureIndex = 0;
        TextureIndex specularTextureIndex = 0;
    };

    std::vector<vkl::VertexLayout> vertices;
    std::vector<uint32_t>          indices;

    std::vector<Image*>       _images;
    std::vector<Node *>       _nodes;
    std::vector<Material>  _materials;

    vkl::ShaderPass *_pass = nullptr;
private:
    void          loadImages(tinygltf::Model &input);
    void          loadMaterials(tinygltf::Model &input);
    void          loadNodes(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                                std::vector<uint32_t> &indices, std::vector<vkl::VertexLayout> &vertices);

protected:
    vklt::EntityLoader *loader;
    vkl::Device        *_device;
};
}

#endif // VKLENTITY_H_
