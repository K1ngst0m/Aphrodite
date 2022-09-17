#ifndef VKLENTITY_H_
#define VKLENTITY_H_

#include "vklObject.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
class Entity : public Object {
public:
    Entity(SceneManager * manager)
        :Object(manager)
    {}

    void pushImage(std::string imagePath, VkQueue queue);
    void setupMesh(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout> &vertices,
                   const std::vector<uint32_t> &indices = {}, size_t vSize = 0, size_t iSize = 0);
    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);
    void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) ;

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() ;

    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f)) ;

    void destroy() override;

private:
    struct Node {
        Node               *parent;
        std::vector<Node *> children;
        vkl::Mesh           mesh;
        glm::mat4           matrix;
    };

    vkl::Texture *getTexture(uint32_t index);
    void          pushImage(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize, VkQueue queue);
    void          drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node);
    void          loadImages(VkQueue queue, tinygltf::Model &input);
    void          loadTextures(tinygltf::Model &input);
    void          loadMaterials(tinygltf::Model &input);
    void          loadNode(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                           std::vector<uint32_t> &indices, std::vector<vkl::VertexLayout> &vertices);

    std::vector<Node *>        _nodes;
    std::vector<vkl::Texture>  _textures;
    std::vector<vkl::Material> _materials;
    vkl::Mesh                  _mesh;

protected:
    vkl::Device *_device;
};
}

#endif // VKLENTITY_H_
