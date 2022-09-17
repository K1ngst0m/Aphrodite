#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "vklObject.h"

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

    // manual mesh
    void pushImage(std::string imagePath, VkQueue queue);
    void loadMeshDevice(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout> &vertices,
                   const std::vector<uint32_t> &indices = {}, size_t vSize = 0, size_t iSize = 0);

    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);
    void loadFromFileLocal(const std::string &path);
    void loadFromFileDevice(vkl::Device *device, VkQueue queue);

    void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) ;

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() ;

    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f)) ;

    void destroy() override;

private:
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
    std::vector<Image>             _images;
    std::vector<Node *>            _nodes;
    std::vector<vkl::Material>     _materials;

    // device data
    std::vector<vkl::Texture> _textures;
    vkl::Mesh                 _mesh;

private:
    vkl::Texture *getTexture(uint32_t index);
    void          pushImageDevice(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize, VkQueue queue);
    void          loadImagesDevice(const std::vector<Image> &images, VkQueue queue);
    void          loadImagesLocal(tinygltf::Model &input);
    void          loadMaterials(tinygltf::Model &input);
    void          loadNodeLocal(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                                std::vector<uint32_t> &indices, std::vector<vkl::VertexLayout> &vertices);
    void          drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node);

protected:
    vklt::EntityLoader *loader;
    vkl::Device        *_device;
};
}

#endif // VKLENTITY_H_
