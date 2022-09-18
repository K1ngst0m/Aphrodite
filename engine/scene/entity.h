#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "object.h"
#include "material.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vklt {
    class EntityLoader;
}

namespace vkl {

class Entity : public Object {
public:
    Entity(SceneManager * manager)
        :Object(manager)
    {}
    void loadFromFile(const std::string &path);
    void destroy() override;

    // TODO move to scene renderer
    void pushImage(std::string imagePath, VkQueue queue);
    void loadMesh(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout> &vertices,
                   const std::vector<uint32_t> &indices = {}, size_t vSize = 0, size_t iSize = 0);
    vkl::Mesh                 _mesh;
    std::vector<vkl::Texture> _textures;
    std::vector<VkDescriptorSet> materialSets;

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

    // local data
    std::vector<uint32_t>          indices;
    std::vector<vkl::VertexLayout> vertices;
    std::vector<Image*>            _images;
    std::vector<Node *>            _nodes;
    std::vector<vkl::Material>     _materials;

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
