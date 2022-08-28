#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "vklUtils.h"
#include "vklMesh.h"
#include "vklMaterial.h"
#include "vklDevice.h"
#include "vklInit.hpp"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
struct Model {
    VkBuffer getVertexBuffer(){ return _mesh.getVertexBuffer(); }

    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    void setupImageDescriptorSet(VkDescriptorSetLayout layout);

    void destroy() const;
private:
    struct Node {
        Node* parent;
        std::vector<Node*> children;
        vkl::Mesh mesh;
        glm::mat4 matrix;
    };

    struct Image{
        vkl::Texture texture;
        VkDescriptorSet descriptorSet;
    };

    struct TextureRef{
        int32_t index;
    };

    vkl::Device * _device;

    std::vector<Image> _images;
    std::vector<TextureRef> _textureRefs;

    vkl::Mesh _mesh;
    std::vector<vkl::Material> _materials;
    std::vector<Node*> _nodes;

    VkDescriptorPool _descriptorPool;

    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node);

    void loadImages(VkQueue queue, tinygltf::Model &input);

    void loadTextures(tinygltf::Model &input);

    void loadMaterials(tinygltf::Model &input);

    void loadNode(const tinygltf::Node &inputNode,
                  const tinygltf::Model &input,
                  Node *parent,
                  std::vector<uint32_t> &indices,
                  std::vector<vkl::VertexLayout> &vertices);
};

}


#endif // VKLMODEL_H_
