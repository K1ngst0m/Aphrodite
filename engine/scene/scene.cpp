#include "scene.h"
#include "common/assetManager.h"
#include "common/common.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace aph::gltf
{
void loadImages(std::vector<std::shared_ptr<ImageInfo>>& images, tinygltf::Model& input)
{
    images.clear();
    for(auto& glTFImage : input.images)
    {
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        auto newImage    = std::make_shared<ImageInfo>();
        newImage->width  = glTFImage.width;
        newImage->height = glTFImage.height;
        newImage->data.resize(glTFImage.width * glTFImage.height * 4);
        newImage->format = Format::R8G8B8A8_UNORM;
        if(glTFImage.component == 3)
        {
            std::vector<uint8_t> rgba(newImage->data.size());
            for(size_t i = 0; i < glTFImage.width * glTFImage.height; ++i)
            {
                memcpy(&rgba[4 * i], &glTFImage.image[3 * i], 3);
            }
            memcpy(newImage->data.data(), rgba.data(), glTFImage.image.size());
        }
        else { memcpy(newImage->data.data(), glTFImage.image.data(), glTFImage.image.size()); }
        images.push_back(newImage);
    }
}

void loadMaterials(std::vector<Material>& materials, tinygltf::Model& input, uint32_t offset)
{
    materials.clear();
    materials.resize(input.materials.size());
    for(size_t i = 0; i < input.materials.size(); i++)
    {
        auto& material = materials[i];
        material.id    = i;
        tinygltf::Material glTFMaterial{input.materials[i]};

        // factor
        material.emissiveFactor  = glm::vec4(glm::make_vec3(glTFMaterial.emissiveFactor.data()), 1.0f);
        material.baseColorFactor = glm::make_vec4(glTFMaterial.pbrMetallicRoughness.baseColorFactor.data());
        material.metallicFactor  = glTFMaterial.pbrMetallicRoughness.metallicFactor;
        material.roughnessFactor = glTFMaterial.pbrMetallicRoughness.roughnessFactor;

        material.doubleSided = glTFMaterial.doubleSided;
        if(glTFMaterial.alphaMode == "BLEND") { material.alphaMode = AlphaMode::BLEND; }
        if(glTFMaterial.alphaMode == "MASK")
        {
            material.alphaCutoff = 0.5f;
            material.alphaMode   = AlphaMode::MASK;
        }
        material.alphaCutoff = glTFMaterial.alphaCutoff;

        // common texture
        if(glTFMaterial.normalTexture.index > -1)
        {
            material.normalId = input.textures[glTFMaterial.normalTexture.index].source + offset;
        }
        if(glTFMaterial.emissiveTexture.index > -1)
        {
            material.emissiveId = input.textures[glTFMaterial.emissiveTexture.index].source + offset;
        }
        if(glTFMaterial.occlusionTexture.index > -1)
        {
            material.occlusionId = input.textures[glTFMaterial.occlusionTexture.index].source + offset;
        }

        // pbr texture
        if(glTFMaterial.pbrMetallicRoughness.baseColorTexture.index > -1)
        {
            material.baseColorId =
                input.textures[glTFMaterial.pbrMetallicRoughness.baseColorTexture.index].source + offset;
        }
        if(glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
        {
            material.metallicRoughnessId =
                input.textures[glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index].source + offset;
        }
    }
}

void loadNodes(std::vector<uint8_t>& verticesList, std::vector<uint8_t>& indicesList, const tinygltf::Node& inputNode,
               const tinygltf::Model& input, const std::shared_ptr<SceneNode>& parent, uint32_t materialOffset)
{
    glm::mat4 matrix{1.0f};

    if(inputNode.translation.size() == 3)
    {
        matrix = glm::translate(matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if(inputNode.rotation.size() == 4)
    {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        matrix *= glm::mat4(q);
    }
    if(inputNode.scale.size() == 3) { matrix = glm::scale(matrix, glm::vec3(glm::make_vec3(inputNode.scale.data()))); }
    if(inputNode.matrix.size() == 16) { matrix = glm::make_mat4x4(inputNode.matrix.data()); };

    auto node{parent->createChildNode(matrix, inputNode.name)};

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if(inputNode.mesh > -1)
    {
        auto mesh{Object::Create<Mesh>()};
        node->attachObject<Mesh>(mesh);

        const tinygltf::Mesh gltfMesh{input.meshes[inputNode.mesh]};
        std::vector<uint8_t> indices;
        std::vector<uint8_t> vertices;
        auto                 indexType{IndexType::UINT16};

        // Iterate through all primitives of this node's mesh
        for(const auto& glTFPrimitive : gltfMesh.primitives)
        {
            auto firstIndex{static_cast<int32_t>(indices.size())};
            auto vertexStart{static_cast<int32_t>(vertices.size())};
            auto indexCount{static_cast<int32_t>(0)};
            auto vertexCount{0};

            // Vertices
            {
                const float* positionBuffer{};
                const float* normalsBuffer{};
                const float* texCoordsBuffer{};
                const float* tangentsBuffer{};

                // Get buffer data for vertex normals
                if(glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor =
                        input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    positionBuffer                   = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if(glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor =
                        input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    normalsBuffer                    = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if(glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor =
                        input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer                  = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                if(glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor =
                        input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    tangentsBuffer                   = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for(size_t v = 0; v < vertexCount; v++)
                {
                    Vertex vert{};
                    vert.pos    = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(
                        glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv      = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color   = glm::vec3(1.0f);
                    vert.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
                    uint8_t* ptr = reinterpret_cast<uint8_t*>(&vert);
                    std::copy(ptr, ptr + sizeof(vert), back_inserter(vertices));
                }
            }
            // Indices
            {
                const tinygltf::Accessor&   accessor   = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                uint32_t idxOffset{static_cast<uint32_t>(indices.size())};
                switch(accessor.componentType)
                {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                {
                    indices.resize(indices.size() + accessor.count * 4);
                    auto* dataPtr = reinterpret_cast<uint32_t*>(&indices[idxOffset]);
                    indexType     = IndexType::UINT32;
                    const auto* buf =
                        reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        dataPtr[index] = buf[index] + vertexStart;
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                {
                    indices.resize(indices.size() + accessor.count * 2);
                    auto* dataPtr = reinterpret_cast<uint16_t*>(&indices[idxOffset]);
                    indexType     = IndexType::UINT16;
                    const auto* buf =
                        reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        dataPtr[index] = buf[index] + vertexStart;
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }

            mesh->m_subsets.push_back(Mesh::Subset{
                .firstIndex    = firstIndex,
                .firstVertex   = vertexStart,
                .vertexCount   = vertexCount,
                .indexCount    = indexCount,
                .materialIndex = static_cast<ResourceIndex>(glTFPrimitive.material + materialOffset),
                .hasIndices    = indexCount > 0,
            });
        }

        static float indexSizeScaling = 1.0f;

        mesh->m_indexType   = indexType;
        mesh->m_indexOffset = indicesList.size() * indexSizeScaling;
        // TODO variable vertex form
        mesh->m_vertexOffset = verticesList.size() / sizeof(Vertex);

        indicesList.insert(indicesList.cend(), indices.cbegin(), indices.cend());
        verticesList.insert(verticesList.cend(), vertices.cbegin(), vertices.cend());

        switch(indexType)
        {
        case IndexType::UINT16: indexSizeScaling = 0.5f; break;
        case IndexType::UINT32: indexSizeScaling = 0.25f; break;
        default: indexSizeScaling = 1.0f; break;
        }
    }

    // Load node's children
    if(!inputNode.children.empty())
    {
        for(const int nodeIdx : inputNode.children)
        {
            loadNodes(verticesList, indicesList, input.nodes[nodeIdx], input, node, materialOffset);
        }
    }
}
}  // namespace aph::gltf

namespace aph
{

std::unique_ptr<Scene> Scene::Create(SceneType type)
{
    switch(type)
    {
    case SceneType::DEFAULT:
    {
        auto instance{std::unique_ptr<Scene>(new Scene())};
        instance->m_rootNode = std::make_shared<SceneNode>(nullptr);
        return instance;
    }
    default:
    {
        assert("scene manager type not support.");
        return {};
    }
    }
}

std::shared_ptr<Light> Scene::createLight()
{
    auto light               = Object::Create<Light>();
    m_lights[light->getId()] = light;
    return light;
}

std::shared_ptr<Mesh> Scene::createMesh()
{
    auto mesh               = Object::Create<Mesh>();
    m_meshes[mesh->getId()] = mesh;
    return mesh;
}

std::shared_ptr<SceneNode> Scene::createMeshesFromFile(const std::string&                path,
                                                       const std::shared_ptr<SceneNode>& parent)
{
    auto node = parent ? parent->createChildNode() : m_rootNode->createChildNode();

    tinygltf::Model    inputModel;
    tinygltf::TinyGLTF gltfContext;
    std::string        error, warning;

    bool fileLoaded = false;
    if(path.find(".glb") != std::string::npos)
    {
        fileLoaded = gltfContext.LoadBinaryFromFile(&inputModel, &error, &warning, path);
    }
    else { fileLoaded = gltfContext.LoadASCIIFromFile(&inputModel, &error, &warning, path); }

    if(fileLoaded)
    {
        const uint32_t                          imageOffset    = m_images.size();
        const uint32_t                          materialOffset = m_materials.size();
        std::vector<std::shared_ptr<ImageInfo>> images;
        std::vector<Material>                   materials;
        gltf::loadImages(images, inputModel);
        gltf::loadMaterials(materials, inputModel, imageOffset);
        m_images.insert(m_images.cend(), std::make_move_iterator(images.cbegin()),
                        std::make_move_iterator(images.cend()));
        m_materials.insert(m_materials.cend(), std::make_move_iterator(materials.cbegin()),
                           std::make_move_iterator(materials.cend()));

        const tinygltf::Scene& scene = inputModel.scenes[0];
        for(int nodeIdx : scene.nodes)
        {
            const tinygltf::Node inputNode = inputModel.nodes[nodeIdx];
            gltf::loadNodes(m_vertices, m_indices, inputNode, inputModel, node, materialOffset);
        }
    }
    else
    {
        std::cout << error << std::endl;
        assert("Could not open the glTF file.");
        return {};
    }

    return node;
}
std::shared_ptr<OrthoCamera> Scene::createOrthoCamera(float aspectRatio)
{
    auto camera                = Object::Create<OrthoCamera>();
    camera->m_aspect           = {aspectRatio};
    m_cameras[camera->getId()] = camera;
    return camera;
}
std::shared_ptr<PerspectiveCamera> Scene::createPerspectiveCamera(float aspectRatio)
{
    auto camera                = Object::Create<PerspectiveCamera>();
    camera->m_aspect           = {aspectRatio};
    m_cameras[camera->getId()] = camera;
    return camera;
}
void Scene::update(float deltaTime)
{
    auto camera = getMainCamera();
    camera->updateProj();
    camera->updateView();
    camera->updateMovement(deltaTime);
}
}  // namespace aph
