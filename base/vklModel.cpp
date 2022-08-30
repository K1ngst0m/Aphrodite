#define TINYGLTF_IMPLEMENTATION
#include "vklModel.h"

namespace vkl {

void Model::loadImages(VkQueue queue, tinygltf::Model &input)
{
    // Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
    // loading them from disk, we fetch them from the glTF loader and upload the buffers
    for (auto & glTFImage : input.images) {
         // Get the image data from the glTF loader
        unsigned char *imageData = nullptr;
        VkDeviceSize imageDataSize = 0;
        bool deleteBuffer = false;
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        if (glTFImage.component == 3) {
            imageDataSize = glTFImage.width * glTFImage.height * 4;
            imageData = new unsigned char[imageDataSize];
            unsigned char *rgba = imageData;
            unsigned char *rgb = glTFImage.image.data();
            for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        } else {
            imageData = glTFImage.image.data();
            imageDataSize = glTFImage.image.size();
        }

        pushImage(glTFImage.width, glTFImage.height, imageData, imageDataSize, queue);

        if (deleteBuffer) {
            delete[] imageData;
        }
    }
}
void Model::loadTextures(tinygltf::Model &input)
{
    _textureRefs.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        _textureRefs[i].index = input.textures[i].source;
    }
}
void Model::loadMaterials(tinygltf::Model &input)
{
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
void Model::loadNode(const tinygltf::Node &inputNode,
                     const tinygltf::Model &input,
                     Node *parent,
                     std::vector<uint32_t> &indices,
                     std::vector<vkl::VertexLayout> &vertices)
{
    Node* node = new Node();
    node->matrix = glm::mat4(1.0f);
    node->parent = parent;

    // Get the local node matrix
    // It's either made up from translation, rotation, scale or a 4x4 matrix
    if (inputNode.translation.size() == 3) {
        node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if (inputNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node->matrix *= glm::mat4(q);
    }
    if (inputNode.scale.size() == 3) {
        node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if (inputNode.matrix.size() == 16) {
        node->matrix = glm::make_mat4x4(inputNode.matrix.data());
    };

    // Load node's children
    if (!inputNode.children.empty()) {
        for (int nodeIdx : inputNode.children) {
            loadNode(input.nodes[nodeIdx], input, node, indices, vertices);
        }
    }

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if (inputNode.mesh > -1) {
        const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
        // Iterate through all primitives of this node's mesh
        for (const auto &glTFPrimitive : mesh.primitives) {
            auto firstIndex = static_cast<uint32_t>(indices.size());
            auto vertexStart = static_cast<uint32_t>(vertices.size());
            auto indexCount = static_cast<uint32_t>(0);

            // Vertices
            {
                const float *positionBuffer = nullptr;
                const float *normalsBuffer = nullptr;
                const float *texCoordsBuffer = nullptr;
                size_t vertexCount = 0;

                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    vkl::VertexLayout vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color = glm::vec3(1.0f);
                    vertices.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const auto *buf = reinterpret_cast<const uint32_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const auto *buf = reinterpret_cast<const uint16_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const auto *buf = reinterpret_cast<const uint8_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }

            node->mesh.pushPrimitive(firstIndex, indexCount, glTFPrimitive.material);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        _nodes.push_back(node);
    }
}
void Model::drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const Node *node)
{
    if (!node->mesh.primitives.empty()) {
        // Pass the node's matrix via push constants
        // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
        glm::mat4 nodeMatrix = node->matrix;
        Node *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        // Pass the final matrix to the vertex shader using push constants
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
        for (const vkl::Primitive &primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                TextureRef textureRef = _textureRefs[_materials[primitive.materialIndex].baseColorTextureIndex];
                // Bind the descriptor for the current primitive's texture
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[textureRef.index].descriptorSet, 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (Node *child : node->children) {
        drawNode(commandBuffer, pipelineLayout, child);
    }
}
void Model::draw(VkCommandBuffer commandBuffer, DrawContextDirtyBits dirtyBits)
{
    assert(_pass);
    // All vertices and indices are stored in single buffers, so we only need to bind once
    VkDeviceSize offsets[1] = { 0 };
    if (dirtyBits & DRAWCONTEXT_VERTEX_BUFFER){
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
    }

    if (dirtyBits & DRAWCONTEXT_INDEX_BUFFER){
        vkCmdBindIndexBuffer(commandBuffer, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    if (dirtyBits & DRAWCONTEXT_PIPELINE){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pass->builtPipeline);
    }
    // Render all nodes at top-level
    for (Node *node : _nodes) {
        node->matrix = transform;
        drawNode(commandBuffer, _pass->layout, node);
    }
}
void Model::destroy()
{
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    _mesh.destroy();

    for (const auto &image : _images) {
        image.texture.destroy();
    }
}
void Model::loadFromFile(vkl::Device *device, VkQueue queue, const std::string &path)
{
    _device = device;

    tinygltf::Model glTFInput;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, path);

    std::vector<uint32_t> indices;
    std::vector<vkl::VertexLayout> vertices;

    assert(_device);
    if (fileLoaded) {
        loadImages(queue, glTFInput);
        loadMaterials(glTFInput);
        loadTextures(glTFInput);
        const tinygltf::Scene &scene = glTFInput.scenes[0];
        for (int nodeIdx : scene.nodes) {
            const tinygltf::Node node = glTFInput.nodes[nodeIdx];
            loadNode(node, glTFInput, nullptr, indices, vertices);
        }
    } else {
        assert("Could not open the glTF file.");
        return;
    }

    // Create and upload vertex and index buffer
    size_t vertexBufferSize = vertices.size() * sizeof(vkl::VertexLayout);
    size_t indexBufferSize = indices.size() * sizeof(indices[0]);

    MeshObject::setupMesh(device, queue, vertices, indices, vertexBufferSize, indexBufferSize);
}
void MeshObject::pushImage(uint32_t width, uint32_t height, unsigned char *imageData, VkDeviceSize imageDataSize, VkQueue queue)
{
    // Load texture from image buffer
    vkl::Buffer stagingBuffer;
    _device->createBuffer(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    stagingBuffer.map();
    stagingBuffer.copyTo(imageData, static_cast<size_t>(imageDataSize));
    stagingBuffer.unmap();

    Image image;
    _device->createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         image.texture);

    _device->transitionImageLayout(queue, image.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _device->copyBufferToImage(queue, stagingBuffer.buffer, image.texture.image, width, height);
    _device->transitionImageLayout(queue, image.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    image.texture.view = _device->createImageView(image.texture.image, VK_FORMAT_R8G8B8A8_SRGB);
    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
    VK_CHECK_RESULT(vkCreateSampler(_device->logicalDevice, &samplerInfo, nullptr, &image.texture.sampler));
    image.texture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _images.push_back(image);

    stagingBuffer.destroy();
}
void MeshObject::pushImage(std::string imagePath, VkQueue queue)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(imagePath.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    assert(pixels && "read texture failed.");

    vkl::Buffer stagingBuffer;
    _device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    stagingBuffer.map();
    stagingBuffer.copyTo(pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    Image image;
    _device->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         image.texture);

    _device->transitionImageLayout(queue, image.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _device->copyBufferToImage(queue, stagingBuffer.buffer, image.texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    _device->transitionImageLayout(queue, image.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    image.texture.view = _device->createImageView(image.texture.image, VK_FORMAT_R8G8B8A8_SRGB);
    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
    VK_CHECK_RESULT(vkCreateSampler(_device->logicalDevice, &samplerInfo, nullptr, &image.texture.sampler));
    image.texture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _images.push_back(image);

    stagingBuffer.destroy();
}
void MeshObject::draw(VkCommandBuffer commandBuffer, DrawContextDirtyBits dirtyBits)
{
    assert(_pass);
    VkDeviceSize offsets[1] = { 0 };

    if (dirtyBits & DRAWCONTEXT_VERTEX_BUFFER){
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
    }

    if (dirtyBits & DRAWCONTEXT_INDEX_BUFFER){
        vkCmdBindIndexBuffer(commandBuffer, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    if (dirtyBits & DRAWCONTEXT_PUSH_CONSTANT){
        vkCmdPushConstants(commandBuffer, _pass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
    }

    if (dirtyBits & DRAWCONTEXT_GLOBAL_SET){
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pass->layout, 1, 1, &_images[0].descriptorSet, 0, nullptr);
    }

    if (dirtyBits & DRAWCONTEXT_PIPELINE){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pass->builtPipeline);
    }

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(getIndicesCount()), 1, 0, 0, 0);
}
void MeshObject::setupDescriptor(VkDescriptorSetLayout layout)
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(_images.size()) },
    };

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, static_cast<uint32_t>(_images.size()));

    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    for (auto &image : _images) {
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(_descriptorPool, &layout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &image.descriptorSet));
        VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(image.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image.texture.descriptorInfo);
        vkUpdateDescriptorSets(_device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }
}
void MeshObject::destroy()
{
    _mesh.destroy();

    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);

    for (auto& image : _images){
        image.texture.destroy();
    }
}
void MeshObject::setupMesh(vkl::Device *device, VkQueue queue,
                           const std::vector<VertexLayout> &vertices,
                           const std::vector<uint32_t> &indices,
                           size_t vSize, size_t iSize)
{
    _device = device;
    _mesh.setup(_device, queue, vertices, indices, vSize, iSize);
}
void MeshObject::setShaderPass(vkl::ShaderPass *pass)
{
    _pass = pass;
}
}
