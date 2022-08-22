#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "vklUtils.h"
#include "vklMesh.h"
#include "vklMaterial.h"
#include "vklDevice.h"
#include "vklInit.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
struct Model {
    // A primitive contains the data for a single draw call
    struct Node {
        Node* parent;
        std::vector<Node> children;
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

    std::vector<Image> _images;
    std::vector<TextureRef> _textureRefs;

    vkl::Mesh _mesh;
    std::vector<vkl::Material> _materials;
    std::vector<Node> nodes;

    void loadImages(vkl::Device *device, VkQueue queue, tinygltf::Model &input);

    void loadTextures(tinygltf::Model &input);

    void loadMaterials(tinygltf::Model &input);

    void loadNode(const tinygltf::Node &inputNode,
                  const tinygltf::Model &input,
                  Node *parent,
                  std::vector<uint32_t> &indexBuffer,
                  std::vector<vkl::VertexLayout> &vertexBuffer);

    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node &node);

    // Draw the glTF scene starting at the top-level-nodes
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    void destroy() const;
};

}


#endif // VKLMODEL_H_
