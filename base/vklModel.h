#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "vklUtils.h"
#include "vklMesh.h"
#include "vklMaterial.h"
#include "vklDevice.h"
#include "vklInit.hpp"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <stb_image.h>

namespace vkl {

class IBaseObject{
public:
    virtual void destroy() = 0;
};

class UniformBufferObject : IBaseObject{
public:
    vkl::UniformBuffer buffer;

    void setupBuffer(vkl::Device * device, VkDeviceSize bufferSize, void * data = nullptr){
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, data);
        buffer.setupDescriptor();
    }

    void update(const void* data){
        buffer.update(data);
    }

    void destroy() override{
        buffer.destroy();
    }
};

class RenderObject: IBaseObject{
public:
    virtual void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) = 0;

    void setupTransform(glm::mat4 matrix){
        transform = matrix;
    }

protected:
    vkl::Device * _device;

    glm::mat4 transform = glm::mat4(1.0f);
};

class MeshObject : public RenderObject{
public:
    void setupMesh(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout>& vertices, const std::vector<uint32_t>& indices = {}, size_t vSize = 0, size_t iSize = 0){
        _device = device;
        _mesh.setup(_device, queue, vertices, indices, vSize, iSize);
    }


    void destroy() override;

    void pushImage(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize, VkQueue queue);
    void pushImage(std::string imagePath, VkQueue queue);

    VkBuffer getVertexBuffer() const { return _mesh.getVertexBuffer(); }
    VkBuffer getIndexBuffer() const { return _mesh.getIndexBuffer(); }

    uint32_t getVerticesCount() const { return _mesh.getVerticesCount(); }
    uint32_t getIndicesCount() const { return _mesh.getIndicesCount(); }

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) override;

    virtual void setupDescriptor(VkDescriptorSetLayout layout);

    vkl::Mesh _mesh;

    VkDescriptorPool _descriptorPool;

    struct Image{
        vkl::Texture texture;
        VkDescriptorSet descriptorSet;
    };

    std::vector<Image> _images;
};

class Model : public MeshObject{
public:
    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) override;
    void destroy() override;

private:
    struct Node {
        Node* parent;
        std::vector<Node*> children;
        vkl::Mesh mesh;
        glm::mat4 matrix;
    };

    struct TextureRef{
        int32_t index;
    };

    std::vector<TextureRef> _textureRefs;
    std::vector<Node*> _nodes;
    std::vector<vkl::Material> _materials;

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
