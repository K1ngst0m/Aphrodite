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

enum DrawContextDirtyBits : uint8_t {
    DRAWCONTEXT_NONE          = 0b00000,
    DRAWCONTEXT_VERTEX_BUFFER = 0b00001,
    DRAWCONTEXT_INDEX_BUFFER  = 0b00010,
    DRAWCONTEXT_PUSH_CONSTANT = 0b00100,
    DRAWCONTEXT_GLOBAL_SET    = 0b01000,
    DRAWCONTEXT_PIPELINE      = 0b10000,
    DRAWCONTEXT_ALL           = 0b11111,
};

inline void operator|=(DrawContextDirtyBits &lhs, DrawContextDirtyBits rhs) {
    lhs = static_cast<DrawContextDirtyBits>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline DrawContextDirtyBits operator|(DrawContextDirtyBits lhs, DrawContextDirtyBits rhs) {
    return static_cast<DrawContextDirtyBits>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

class Scene;

class IBaseObject {
public:
    virtual void destroy() = 0;
};

class UniformBufferObject : IBaseObject {
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

class RenderObject : IBaseObject {
public:
    virtual void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f),
                      DrawContextDirtyBits dirtyBits = DRAWCONTEXT_ALL) = 0;
    virtual void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool)          = 0;

    virtual std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() = 0;

protected:
    vkl::Device *_device;
};

class MeshObject : public RenderObject {
public:
    friend class vkl::Scene;

    void setupMesh(vkl::Device *device, VkQueue queue, const std::vector<VertexLayout> &vertices,
                   const std::vector<uint32_t> &indices = {}, size_t vSize = 0, size_t iSize = 0);

    void destroy() override;

    void pushImage(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize,
                   VkQueue queue);
    void pushImage(std::string imagePath, VkQueue queue);

    VkBuffer getVertexBuffer() const {return _mesh.getVertexBuffer();}
    VkBuffer getIndexBuffer() const {return _mesh.getIndexBuffer();}
    uint32_t getVerticesCount() const {return _mesh.getVerticesCount();}
    uint32_t getIndicesCount() const {return _mesh.getIndicesCount();}

    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f),
              DrawContextDirtyBits dirtyBits = DRAWCONTEXT_ALL) override;

    void setupDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) override;

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() override{
        std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(_images.size())},
        };

        return poolSizes;
    }

protected:
    struct Image {
        vkl::Texture    texture;
        VkDescriptorSet descriptorSet;
    };

    vkl::Mesh                  _mesh;
    std::vector<Image>         _images;
    std::vector<vkl::Material> _materials;
};

class Model : public MeshObject {
public:
    void loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path);
    void draw(VkCommandBuffer commandBuffer, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f),
              DrawContextDirtyBits dirtyBits = DRAWCONTEXT_ALL) override;
    void destroy() override;

private:
    struct Node {
        Node               *parent;
        std::vector<Node *> children;
        vkl::Mesh           mesh;
        glm::mat4           matrix;
    };
    std::vector<Node *>     _nodes;

    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node);
    void loadImages(VkQueue queue, tinygltf::Model &input);
    void loadTextures(tinygltf::Model &input);
    void loadMaterials(tinygltf::Model &input);
    void loadNode(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                  std::vector<uint32_t> &indices, std::vector<vkl::VertexLayout> &vertices);
};

} // namespace vkl

#endif // VKLMODEL_H_
