#define TINYGLTF_IMPLEMENTATION
#include "resourceManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "entity.h"

namespace vkl {
void GLTFLoader::load(Entity *entity, const std::string &path) {
    tinygltf::Model    glTFInput;
    tinygltf::TinyGLTF gltfContext;
    std::string        error, warning;

    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, path);

    if (fileLoaded) {
        _loadImages(entity, glTFInput);
        _loadMaterials(entity, glTFInput);

        const tinygltf::Scene &scene = glTFInput.scenes[0];
        for (int nodeIdx : scene.nodes) {
            const tinygltf::Node node = glTFInput.nodes[nodeIdx];
            _loadNodes(entity, node, glTFInput, nullptr);
        }
    } else {
        assert("Could not open the glTF file.");
        return;
    }
}

void GLTFLoader::_loadImages(Entity *entity, tinygltf::Model &input) {
    for (auto &glTFImage : input.images) {
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        Texture newTexture;
        newTexture.width  = glTFImage.width;
        newTexture.height = glTFImage.height;
        newTexture.data.resize(glTFImage.width * glTFImage.height * 4);
        if (glTFImage.component == 3) {
            unsigned char *rgba = new unsigned char[newTexture.data.size()];
            unsigned char *rgb  = glTFImage.image.data();
            for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            memcpy(newTexture.data.data(), rgba, glTFImage.image.size());
        } else {
            memcpy(newTexture.data.data(), glTFImage.image.data(), glTFImage.image.size());
        }
        entity->_images.push_back(newTexture);
    }
}
void GLTFLoader::_loadMaterials(Entity *entity, tinygltf::Model &input) {
    entity->_materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        auto &material                  = entity->_materials[i];
        material.id                     = i;
        tinygltf::Material glTFMaterial = input.materials[i];
        if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
            material.baseColorFactor = glm::make_vec4(glTFMaterial.pbrMetallicRoughness.baseColorFactor.data());
        }
        if (glTFMaterial.pbrMetallicRoughness.baseColorTexture.index > -1) {
            material.baseColorTextureIndex = input.textures[glTFMaterial.pbrMetallicRoughness.baseColorTexture.index].source;
        }
        if (glTFMaterial.normalTexture.index > -1) {
            material.normalTextureIndex = input.textures[glTFMaterial.normalTexture.index].source;
        }
    }
}
void GLTFLoader::_loadNodes(Entity *entity, const tinygltf::Node &inputNode, const tinygltf::Model &input, SubEntity *parent) {
    auto node    = std::make_shared<SubEntity>();
    node->matrix = glm::mat4(1.0f);
    node->parent = parent;
    node->name   = inputNode.name;

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
            _loadNodes(entity, input.nodes[nodeIdx], input, node.get());
        }
    }

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if (inputNode.mesh > -1) {
        const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
        // Iterate through all primitives of this node's mesh
        for (const auto &glTFPrimitive : mesh.primitives) {
            auto firstIndex  = static_cast<int32_t>(entity->_indices.size());
            auto vertexStart = static_cast<int32_t>(entity->_vertices.size());
            auto indexCount  = static_cast<int32_t>(0);

            // Vertices
            {
                const float *positionBuffer  = nullptr;
                const float *normalsBuffer   = nullptr;
                const float *texCoordsBuffer = nullptr;
                const float *tangentsBuffer  = nullptr;
                size_t       vertexCount     = 0;

                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    positionBuffer                   = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    normalsBuffer                    = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer                  = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor   &accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                    const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
                    tangentsBuffer                       = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert{};
                    vert.pos     = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal  = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv      = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color   = glm::vec3(1.0f);
                    vert.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
                    entity->_vertices.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor   &accessor   = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer     &buffer     = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const auto *buf =
                        reinterpret_cast<const uint32_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        entity->_indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const auto *buf =
                        reinterpret_cast<const uint16_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        entity->_indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const auto *buf =
                        reinterpret_cast<const uint8_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        entity->_indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }

            Primitive primitive{
                .firstIndex    = firstIndex,
                .indexCount    = indexCount,
                .materialIndex = glTFPrimitive.material,
            };

            node->primitives.push_back(primitive);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        entity->_subEntityList.push_back(node);
    }
}
} // namespace vkl
