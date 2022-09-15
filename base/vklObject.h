#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "vklDevice.h"
#include "vklInit.hpp"
#include "vklMaterial.h"
#include "vklMesh.h"
#include "vklPipeline.h"
#include "vklUtils.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <stb_image.h>
namespace vkl {

class Scene;

class Object {
public:
    virtual void destroy() = 0;
};

class UniformBufferObject : Object {
public:
    vkl::UniformBuffer buffer;

    void setupBuffer(vkl::Device *device, VkDeviceSize bufferSize, void *data = nullptr) {
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, data);
        buffer.setupDescriptor();
    }

    void update(const void *data) {
        buffer.update(data);
    }

    void destroy() override {
        buffer.destroy();
    }
};

class RenderObject : Object {
public:
    virtual void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f)) = 0;
    virtual void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool)          = 0;

    virtual std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() = 0;

protected:
    vkl::Device *_device;
};

class MeshObject : public RenderObject {
public:
    void setupMesh(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout> &vertices,
                   const std::vector<uint32_t> &indices = {}, size_t vSize = 0, size_t iSize = 0);
    void destroy() override;

    void pushImage(std::string imagePath, VkQueue queue);

    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f)) override;

    void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) override;

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() override{
        std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(_textures.size())},
        };

        return poolSizes;
    }

protected:
    vkl::Mesh                  _mesh;
    std::vector<vkl::Texture>  _textures;
    std::vector<vkl::Material> _materials;
};

class Entity : public MeshObject {
public:
    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);
    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f)) override;
    void destroy() override;

private:
    struct Node {
        Node               *parent;
        std::vector<Node *> children;
        vkl::Mesh           mesh;
        glm::mat4           matrix;
    };
    std::vector<Node *>     _nodes;

    vkl::Texture *getTexture(uint32_t index);
    void pushImage(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize, VkQueue queue);
    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node);
    void loadImages(VkQueue queue, tinygltf::Model &input);
    void loadTextures(tinygltf::Model &input);
    void loadMaterials(tinygltf::Model &input);
    void loadNode(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                  std::vector<uint32_t> &indices, std::vector<vkl::VertexLayout> &vertices);
};

} // namespace vkl

#endif // VKLMODEL_H_
