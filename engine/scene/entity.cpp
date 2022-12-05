#include "entity.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

namespace vkl
{

namespace gltf
{
void loadImages(std::vector<std::shared_ptr<ImageDesc>>& images, tinygltf::Model &input)
{
    images.clear();
    for(auto &glTFImage : input.images)
    {
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        auto newImage = std::make_shared<ImageDesc>();
        newImage->width = glTFImage.width;
        newImage->height = glTFImage.height;
        newImage->data.resize(glTFImage.width * glTFImage.height * 4);
        if(glTFImage.component == 3)
        {
            unsigned char *rgba = new unsigned char[newImage->data.size()];
            unsigned char *rgb = glTFImage.image.data();
            for(size_t i = 0; i < glTFImage.width * glTFImage.height; ++i)
            {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            memcpy(newImage->data.data(), rgba, glTFImage.image.size());
        }
        else
        {
            memcpy(newImage->data.data(), glTFImage.image.data(), glTFImage.image.size());
        }
        images.push_back(newImage);
    }
}
void loadMaterials(std::vector<std::shared_ptr<Material>>& materials, tinygltf::Model &input)
{
    materials.clear();
    materials.resize(input.materials.size());
    for(size_t i = 0; i < input.materials.size(); i++)
    {
        auto &material = materials[i];
        material = std::make_shared<Material>();
        material->id = i;
        tinygltf::Material glTFMaterial = input.materials[i];

        // factor
        material->emissiveFactor = glm::vec4(glm::make_vec3(glTFMaterial.emissiveFactor.data()), 1.0f);
        material->baseColorFactor =
            glm::make_vec4(glTFMaterial.pbrMetallicRoughness.baseColorFactor.data());
        material->metallicFactor = glTFMaterial.pbrMetallicRoughness.metallicFactor;
        material->roughnessFactor = glTFMaterial.pbrMetallicRoughness.roughnessFactor;

        material->doubleSided = glTFMaterial.doubleSided;
        if(glTFMaterial.alphaMode == "BLEND")
        {
            material->alphaMode = AlphaMode::BLEND;
        }
        if(glTFMaterial.alphaMode == "MASK")
        {
            material->alphaCutoff = 0.5f;
            material->alphaMode = AlphaMode::MASK;
        }
        material->alphaCutoff = glTFMaterial.alphaCutoff;

        // common texture
        if(glTFMaterial.normalTexture.index > -1)
        {
            material->normalTextureIndex = input.textures[glTFMaterial.normalTexture.index].source;
        }
        if(glTFMaterial.emissiveTexture.index > -1)
        {
            material->emissiveTextureIndex = input.textures[glTFMaterial.emissiveTexture.index].source;
        }
        if(glTFMaterial.occlusionTexture.index > -1)
        {
            material->occlusionTextureIndex = input.textures[glTFMaterial.occlusionTexture.index].source;
        }

        // pbr texture
        if(glTFMaterial.pbrMetallicRoughness.baseColorTexture.index > -1)
        {
            material->baseColorTextureIndex =
                input.textures[glTFMaterial.pbrMetallicRoughness.baseColorTexture.index].source;
        }
        if(glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
        {
            material->metallicRoughnessTextureIndex =
                input.textures[glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
        }
    }
}
void loadNodes(std::vector<Vertex> &vertices,
               std::vector<uint32_t>& indices,
               const tinygltf::Node &inputNode,
               const tinygltf::Model &input,
               const std::shared_ptr<MeshNode>& parent)
{
    auto node = parent->createChildNode();
    node->matrix = glm::mat4(1.0f);
    node->parent = parent;
    node->name = inputNode.name;

    if(inputNode.translation.size() == 3)
    {
        node->matrix =
            glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if(inputNode.rotation.size() == 4)
    {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node->matrix *= glm::mat4(q);
    }
    if(inputNode.scale.size() == 3)
    {
        node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if(inputNode.matrix.size() == 16)
    {
        node->matrix = glm::make_mat4x4(inputNode.matrix.data());
    };

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if(inputNode.mesh > -1)
    {
        const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
        // Iterate through all primitives of this node's mesh
        for(const auto &glTFPrimitive : mesh.primitives)
        {
            auto firstIndex = static_cast<int32_t>(indices.size());
            auto vertexStart = static_cast<int32_t>(vertices.size());
            auto indexCount = static_cast<int32_t>(0);

            // Vertices
            {
                const float *positionBuffer = nullptr;
                const float *normalsBuffer = nullptr;
                const float *texCoordsBuffer = nullptr;
                const float *tangentsBuffer = nullptr;
                size_t vertexCount = 0;

                // Get buffer data for vertex normals
                if(glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if(glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if(glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                if(glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor &accessor =
                        input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    tangentsBuffer = reinterpret_cast<const float *>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for(size_t v = 0; v < vertexCount; v++)
                {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(
                        normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv =
                        texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color = glm::vec3(1.0f);
                    vert.tangent =
                        tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
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
                switch(accessor.componentType)
                {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                {
                    const auto *buf = reinterpret_cast<const uint32_t *>(
                        &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                {
                    const auto *buf = reinterpret_cast<const uint16_t *>(
                        &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                {
                    const auto *buf = reinterpret_cast<const uint8_t *>(
                        &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!"
                              << std::endl;
                    return;
                }
            }

            Subset subset{ firstIndex, indexCount, glTFPrimitive.material };

            node->subsets.push_back(subset);
        }
    }

    // Load node's children
    if(!inputNode.children.empty())
    {
        for(int nodeIdx : inputNode.children)
        {
            loadNodes(vertices, indices, input.nodes[nodeIdx], input, node);
        }
    }
}
}  // namespace gltf

void Entity::loadFromFile(const std::string &path)
{
    tinygltf::Model glTFInput;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool fileLoaded = false;
    if(path.find(".glb") != std::string::npos)
    {
        fileLoaded = gltfContext.LoadBinaryFromFile(&glTFInput, &error, &warning, path);
    }
    else
    {
        fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, path);
    }

    if(fileLoaded)
    {
        gltf::loadImages(m_images, glTFInput);
        gltf::loadMaterials(m_materials, glTFInput);

        const tinygltf::Scene &scene = glTFInput.scenes[0];
        for(int nodeIdx : scene.nodes)
        {
            const tinygltf::Node node = glTFInput.nodes[nodeIdx];
            gltf::loadNodes(m_vertices, m_indices, node, glTFInput, m_rootNode->createChildNode());
        }
    }
    else
    {
        std::cout << error << std::endl;
        assert("Could not open the glTF file.");
        return;
    }
}
void Entity::cleanupResources()
{
    m_vertices.clear();
    m_indices.clear();
    m_images.clear();
    m_materials.clear();
}
std::shared_ptr<Entity> Entity::Create()
{
    auto instance = std::make_shared<Entity>(Id::generateNewId<Entity>());
    return instance;
}
}  // namespace vkl
