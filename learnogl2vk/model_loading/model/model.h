#ifndef MODEL_H_
#define MODEL_H_

#include "vklBase.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

/*
** - https://learnopengl.com/Lighting/Basic-Lighting
 */

struct DescriptorSetLayouts {
    VkDescriptorSetLayout scene;
    VkDescriptorSetLayout material;
};

// per scene data
// general scene data
struct SceneDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 viewPosition;
};

// point light scene data
struct DirectionalLightDataLayout {
    glm::vec4 direction;

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

// point light scene data
struct PointLightDataLayout {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 attenuationFactor;
};

// per object data
struct ObjectDataLayout {
    glm::mat4 modelMatrix;
};

namespace vkltemp
{

struct Material {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex = 0;
    glm::vec4 specularFactor = glm::vec4(1.0f);
    float shininess = 64.0f;

    vkl::Texture *baseColorTexture = nullptr;
    vkl::Texture *specularTexture = nullptr;

    VkDescriptorSet descriptorSet;

    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
    {
        assert(baseColorTexture);
        assert(specularTexture);

        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorSetLayout,
        };
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

        std::vector<VkDescriptorImageInfo> imageDescriptors{};
        std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

        if (baseColorTexture){
            imageDescriptors.push_back(baseColorTexture->descriptorInfo);
            VkWriteDescriptorSet writeDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &baseColorTexture->descriptorInfo,
            };
            writeDescriptorSets.push_back(writeDescriptorSet);
        }

        if (specularTexture){
            imageDescriptors.push_back(specularTexture->descriptorInfo);
            VkWriteDescriptorSet writeDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &specularTexture->descriptorInfo,
            };
            writeDescriptorSets.push_back(writeDescriptorSet);
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
};

struct Model {
    // A primitive contains the data for a single draw call
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;
    };

    struct Mesh {
        vkl::VertexBuffer vertexBuffer;
        vkl::IndexBuffer indexBuffer;
        std::vector<Primitive> primitives;

        uint32_t getIndicesCount() const{
            return indexBuffer.indices.size();
        }

        void destroy() const{
            vertexBuffer.destroy();
            indexBuffer.destroy();
        }
    };

    struct Node {
        Node* parent;
        std::vector<Node> children;
        Mesh mesh;
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

    Mesh _mesh;
    std::vector<vkltemp::Material> _materials;
    std::vector<Node> nodes;

    void loadImages(vkl::Device *device, VkQueue queue, tinygltf::Model& input){
        // Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
        // loading them from disk, we fetch them from the glTF loader and upload the buffers
        _images.resize(input.images.size());
        for (size_t i = 0; i < input.images.size(); i++) {
            tinygltf::Image& glTFImage = input.images[i];
            // Get the image data from the glTF loader
            unsigned char* imageData = nullptr;
            VkDeviceSize imageDataSize = 0;
            bool deleteBuffer = false;
            // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
            if (glTFImage.component == 3) {
                imageDataSize = glTFImage.width * glTFImage.height * 4;
                imageData = new unsigned char[imageDataSize];
                unsigned char* rgba = imageData;
                unsigned char* rgb = glTFImage.image.data();
                for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                    memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            }
            else {
                imageData = &glTFImage.image[0];
                imageDataSize = glTFImage.image.size();
            }

            // Load texture from image buffer
            vkl::Buffer stagingBuffer;
            device->createBuffer(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(imageData, static_cast<size_t>(imageDataSize));
            stagingBuffer.unmap();

            device->createImage(glTFImage.width, glTFImage.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        _images[i].texture);

            device->transitionImageLayout(queue, _images[i].texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            device->copyBufferToImage(queue, stagingBuffer.buffer, _images[i].texture.image, static_cast<uint32_t>(glTFImage.width), static_cast<uint32_t>(glTFImage.height));
            device->transitionImageLayout(queue, _images[i].texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            _images[i].texture.view = device->createImageView(_images[i].texture.image, VK_FORMAT_R8G8B8A8_SRGB);
            VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
            VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &_images[i].texture.sampler));
            _images[i].texture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            stagingBuffer.destroy();

            if (deleteBuffer) {
                delete[] imageData;
            }
        }
    }

    void loadTextures(tinygltf::Model& input){
        _textureRefs.resize(input.textures.size());
        for (size_t i = 0; i < input.textures.size(); i++) {
            _textureRefs[i].index = input.textures[i].source;
        }
    }

    void loadMaterials(tinygltf::Model& input){
        _materials.resize(input.materials.size());
        for (size_t i = 0; i < input.materials.size(); i++) {
            // We only read the most basic properties required for our sample
            tinygltf::Material glTFMaterial = input.materials[i];
            // Get the base color factor
            if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
                _materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
            }
            // Get base color texture index
            if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
                _materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
            }
        }
    }

    void loadNode(const tinygltf::Node& inputNode,
                  const tinygltf::Model& input,
                  Node* parent,
                  std::vector<uint32_t>& indexBuffer,
                  std::vector<vkl::VertexLayout>& vertexBuffer){
        Node node{};
        node.matrix = glm::mat4(1.0f);

        // Get the local node matrix
        // It's either made up from translation, rotation, scale or a 4x4 matrix
        if (inputNode.translation.size() == 3) {
            node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
        }
        if (inputNode.rotation.size() == 4) {
            glm::quat q = glm::make_quat(inputNode.rotation.data());
            node.matrix *= glm::mat4(q);
        }
        if (inputNode.scale.size() == 3) {
            node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
        }
        if (inputNode.matrix.size() == 16) {
            node.matrix = glm::make_mat4x4(inputNode.matrix.data());
        };

        // Load node's children
        if (!inputNode.children.empty()) {
            for (int nodeIdx : inputNode.children) {
                loadNode(input.nodes[nodeIdx], input , &node, indexBuffer, vertexBuffer);
            }
        }

        // If the node contains mesh data, we load vertices and indices from the buffers
        // In glTF this is done via accessors and buffer views
        if (inputNode.mesh > -1) {
            const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
            // Iterate through all primitives of this node's mesh
            for (const auto & glTFPrimitive : mesh.primitives) {
                uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
                uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
                uint32_t indexCount = 0;
                // Vertices
                {
                    const float* positionBuffer = nullptr;
                    const float* normalsBuffer = nullptr;
                    const float* texCoordsBuffer = nullptr;
                    size_t vertexCount = 0;

                    // Get buffer data for vertex normals
                    if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }
                    // Get buffer data for vertex normals
                    if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }
                    // Get buffer data for vertex texture coordinates
                    // glTF supports multiple sets, we only load the first one
                    if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    // Append data to model's vertex buffer
                    for (size_t v = 0; v < vertexCount; v++) {
                        vkl::VertexLayout vert{};
                        vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                        vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                        vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                        vert.color = glm::vec3(1.0f);
                        vertexBuffer.push_back(vert);
                    }
                }
                // Indices
                {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
                    const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

                    indexCount += static_cast<uint32_t>(accessor.count);

                    // glTF supports different component types of indices
                    switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const auto* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const auto* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const auto* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }
                }

                Primitive primitive{};
                primitive.firstIndex = firstIndex;
                primitive.indexCount = indexCount;
                primitive.materialIndex = glTFPrimitive.material;
                node.mesh.primitives.push_back(primitive);
            }
        }

        if (parent) {
            parent->children.push_back(node);
        }
        else {
            nodes.push_back(node);
        }
    }

    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node& node) {
        if (!node.mesh.primitives.empty()) {
            // Pass the node's matrix via push constants
            // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
            glm::mat4 nodeMatrix = node.matrix;
            Node* currentParent = node.parent;
            while (currentParent) {
                nodeMatrix = currentParent->matrix * nodeMatrix;
                currentParent = currentParent->parent;
            }
            // Pass the final matrix to the vertex shader using push constants
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
            for (const Primitive& primitive : node.mesh.primitives) {
                if (primitive.indexCount > 0) {
                    // Get the texture index for this primitive
                    TextureRef textureRef = _textureRefs[_materials[primitive.materialIndex].baseColorTextureIndex];
                    // Bind the descriptor for the current primitive's texture
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[textureRef.index].descriptorSet, 0, nullptr);
                    vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
                }
            }
        }
        for (const auto& child : node.children) {
            drawNode(commandBuffer, pipelineLayout, child);
        }
    }

    // Draw the glTF scene starting at the top-level-nodes
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
        // All vertices and indices are stored in single buffers, so we only need to bind once
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        // Render all nodes at top-level
        for (auto& node : nodes) {
            drawNode(commandBuffer, pipelineLayout, node);
        }
    }

    void destroy() const{
        _mesh.destroy();

        for(const auto& image : _images){
            image.texture.destroy();
        }
    }
};

}

class model : public vkl::vklBase {
public:
    ~model() override = default;

private:
    void initDerive() override;
    void drawFrame() override;
    void getEnabledFeatures() override;
    void cleanupDerive() override;

private:
    void setupDescriptors();
    void createUniformBuffers();
    void createDescriptorSets();
    void createDescriptorSetLayout();
    void setupPipelineBuilder();
    void createGraphicsPipeline();
    void createSyncObjects();
    void createDescriptorPool();
    void updateUniformBuffer(uint32_t currentFrameIndex);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createPipelineLayout();
    void loadModelFromFile(vkltemp::Model &model, std::string path);
    void loadModel();

private:
    struct PerFrameData{
        vkl::Buffer sceneUB;
        vkl::Buffer pointLightUB;
        vkl::Buffer directionalLightUB;
        VkDescriptorSet descriptorSet;
    };
    std::vector<PerFrameData> m_perFrameData;

    vkltemp::Model m_cubeModel;

    DescriptorSetLayouts m_descriptorSetLayouts;

    vkl::utils::PipelineBuilder m_pipelineBuilder;

    VkPipelineLayout m_modelPipelineLayout;
    VkPipeline m_modelGraphicsPipeline;
};

#endif // MODEL_H_
