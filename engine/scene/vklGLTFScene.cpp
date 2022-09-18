#include "vklGLTFScene.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace vklt {

static FilterMode convertSamplerFilterType(int value) {
    switch (value) {
    case -1:
        return FilterMode::UNDEFINED;
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return FilterMode::NEAREST;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return FilterMode::LINEAR;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return FilterMode::NEAREST_MIPMAP_NEAREST;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return FilterMode::LINEAR_MIPMAP_NEAREST;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return FilterMode::NEAREST_MIPMAP_LINEAR;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return FilterMode::LINEAR_MIPMAP_LINEAR;
    default:
        assert("Invalid sampler filter type value.");
        return FilterMode::UNDEFINED;
    }
}
static SamplerAddressMode convertSamplerAddressMode(int value) {
    switch (value) {
    case 10497:
        return SamplerAddressMode::REPEAT;
    case 33071:
        return SamplerAddressMode::CLAMP_TO_EDGE;
    case 33648:
        return SamplerAddressMode::MIRRORED_REPEAT;
    default:
        assert("Invalid sampler wrap value.");
        return SamplerAddressMode::REPEAT;
    }
}
static PrimitiveMode convertPrimitiveMode(int value) {
    assert(value >= 0 && value <= 6);
    return static_cast<PrimitiveMode>(value);
}

void EntityLoader::loadSceneFromFile(const std::string &filename, uint32_t fileLoadingFlags, float globalScale) {
    tinygltf::Model    gltfModel;
    tinygltf::TinyGLTF gltfContext;

    std::string error, warning;

    bool   binary = false;
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos) {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
    }

    bool fileLoaded = binary
                          ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename)
                          : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

    LoaderInfo loaderInfo{};
    size_t     vertexCount = 0;
    size_t     indexCount  = 0;

    if (fileLoaded) {
        loadTextures(gltfModel);
        loadImages(gltfModel);
        loadMaterials(gltfModel);

        const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
        // Get vertex and index buffer sizes up-front
        for (int node : scene.nodes) {
            getNodeProps(gltfModel.nodes[node], gltfModel, vertexCount, indexCount);
        }

        loaderInfo.vertexBuffer = new Vertex[vertexCount];
        loaderInfo.indexBuffer  = new uint32_t[indexCount];
        loaderInfo.vertexCount = vertexCount;
        loaderInfo.indexCount = indexCount;

        for (int n : scene.nodes) {
            const tinygltf::Node node = gltfModel.nodes[n];
            loadNode(nullptr, node, n, gltfModel, loaderInfo, globalScale);
        }

        if (!gltfModel.animations.empty()) {
            loadAnimations(gltfModel);
        }
        loadSkins(gltfModel);

    } else {
        assert("Failed to load scene from gltf file.");
    }
}
void EntityLoader::loadMaterials(tinygltf::Model &gltfModel) {
    for (tinygltf::Material &gltfMaterial : gltfModel.materials) {
        Material newMaterial{};
        newMaterial.doubleSided = gltfMaterial.doubleSided;
        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            newMaterial.baseColorTexture       = &textures[gltfMaterial.values["baseColorTexture"].TextureIndex()];
            newMaterial.texCoordSets.baseColor = gltfMaterial.values["baseColorTexture"].TextureTexCoord();
        }
        if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end()) {
            newMaterial.metallicRoughnessTexture       = &textures[gltfMaterial.values["metallicRoughnessTexture"].TextureIndex()];
            newMaterial.texCoordSets.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
        }
        if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end()) {
            newMaterial.roughnessFactor = static_cast<float>(gltfMaterial.values["roughnessFactor"].Factor());
        }
        if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end()) {
            newMaterial.metallicFactor = static_cast<float>(gltfMaterial.values["metallicFactor"].Factor());
        }
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            newMaterial.baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }
        if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end()) {
            newMaterial.normalTexture       = &textures[gltfMaterial.additionalValues["normalTexture"].TextureIndex()];
            newMaterial.texCoordSets.normal = gltfMaterial.additionalValues["normalTexture"].TextureTexCoord();
        }
        if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end()) {
            newMaterial.emissiveTexture       = &textures[gltfMaterial.additionalValues["emissiveTexture"].TextureIndex()];
            newMaterial.texCoordSets.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
        }
        if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end()) {
            newMaterial.occlusionTexture       = &textures[gltfMaterial.additionalValues["occlusionTexture"].TextureIndex()];
            newMaterial.texCoordSets.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureTexCoord();
        }
        if (gltfMaterial.additionalValues.find("alphaMode") != gltfMaterial.additionalValues.end()) {
            tinygltf::Parameter param = gltfMaterial.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") {
                newMaterial.alphaMode = Material::ALPHAMODE_BLEND;
            }
            if (param.string_value == "MASK") {
                newMaterial.alphaCutoff = 0.5f;
                newMaterial.alphaMode   = Material::ALPHAMODE_MASK;
            }
        }
        if (gltfMaterial.additionalValues.find("alphaCutoff") != gltfMaterial.additionalValues.end()) {
            newMaterial.alphaCutoff = static_cast<float>(gltfMaterial.additionalValues["alphaCutoff"].Factor());
        }
        if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end()) {
            newMaterial.emissiveFactor = glm::vec4(glm::make_vec3(gltfMaterial.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
        }

        materials.push_back(newMaterial);
    }
}
void EntityLoader::loadTextures(const tinygltf::Model &gltfModel) {
    for (const tinygltf::Texture &gltfTexture : gltfModel.textures) {
        TextureSampler newSampler;

        // create default texture sampler
        if (gltfTexture.sampler == -1){
            newSampler.magFilter = FilterMode::LINEAR;
            newSampler.minFilter = FilterMode::LINEAR;
            newSampler.wrapS = SamplerAddressMode::REPEAT;
            newSampler.wrapT = SamplerAddressMode::REPEAT;
        }
        else{
            auto gltfSampler = gltfModel.samplers[gltfTexture.sampler];
            newSampler.minFilter = convertSamplerFilterType(gltfSampler.minFilter);
            newSampler.magFilter = convertSamplerFilterType(gltfSampler.magFilter);
            newSampler.wrapS     = convertSamplerAddressMode(gltfSampler.wrapS);
            newSampler.wrapT     = convertSamplerAddressMode(gltfSampler.wrapT);
        }

        Texture texture;
        texture.sampler = newSampler;
        texture.image  = &images[gltfTexture.source];
        textures.push_back(texture);
    }
}
void EntityLoader::loadImages(const tinygltf::Model &gltfModel) {
    for (const tinygltf::Image &gltfImage : gltfModel.images) {
        Image newImage{};

        if (gltfImage.component == 3) {
            newImage.dataSize         = gltfImage.width * gltfImage.height * 4;
            unsigned char       *rgba = newImage.data;
            const unsigned char *rgb  = gltfImage.image.data();
            for (int32_t i = 0; i < gltfImage.width * gltfImage.height; ++i) {
                for (int32_t j = 0; j < 3; ++j) {
                    rgba[j] = rgb[j];
                }
                rgba += 4;
                rgb += 3;
            }
        } else {
            newImage.dataSize = gltfImage.image.size();
            memcpy(newImage.data, gltfImage.image.data(), newImage.dataSize);
        }

        newImage.width      = gltfImage.width;
        newImage.height     = gltfImage.height;
        images.push_back(newImage);
    }
}
void EntityLoader::loadNode(Node *parent, const tinygltf::Node &gltfNode, uint32_t nodeIndex, const tinygltf::Model &gltfModel, LoaderInfo &loaderInfo, float globalscale) {
    Node *newNode      = new Node{};
    newNode->index     = nodeIndex;
    newNode->parent    = parent;
    newNode->name      = gltfNode.name;
    newNode->skinIndex = gltfNode.skin;
    newNode->matrix    = glm::mat4(1.0f);

    if (gltfNode.mesh > -1){
        const tinygltf::Mesh &gltfMesh = gltfModel.meshes[gltfNode.mesh];
        Mesh * newMesh = new Mesh;
        newMesh->name = gltfMesh.name;
        for (const tinygltf::Primitive &gltfPrimitive : gltfMesh.primitives) {
            uint32_t  vertexStart = loaderInfo.vertexPos;
            uint32_t  indexStart  = loaderInfo.indexPos;
            uint32_t  indexCount  = 0;
            uint32_t  vertexCount = 0;
            glm::vec3 posMin{};
            glm::vec3 posMax{};
            bool      hasSkin    = false;
            bool      hasIndices = gltfPrimitive.indices > -1;

            const float *bufferPos          = nullptr;
            const float *bufferNormals      = nullptr;
            const float *bufferTexCoordSet0 = nullptr;
            const float *bufferTexCoordSet1 = nullptr;
            const float *bufferColorSet0    = nullptr;
            const void  *bufferJoints       = nullptr;
            const float *bufferWeights      = nullptr;

            int posByteStride;
            int normByteStride;
            int uv0ByteStride;
            int uv1ByteStride;
            int color0ByteStride;
            int jointByteStride;
            int weightByteStride = 0;

            // Position attribute is required
            assert(gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end());

            for (const auto &[attrName, accessorIdx] : gltfPrimitive.attributes) {
                const tinygltf::Accessor   &accessor = gltfModel.accessors[accessorIdx];
                const tinygltf::BufferView &view     = gltfModel.bufferViews[accessor.bufferView];

                if (attrName == "POSITION") {
                    bufferPos     = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    posMin        = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
                    posMax        = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
                    vertexCount   = static_cast<uint32_t>(accessor.count);
                    posByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                }

                if (attrName == "Normal") {
                    bufferNormals  = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    normByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                }

                if (attrName == "TEXCOORD_0") {
                    bufferTexCoordSet0 = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    uv0ByteStride      = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
                }

                if (attrName == "TEXCOORD_1") {
                    bufferTexCoordSet1 = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    uv1ByteStride      = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
                }

                if (attrName == "COLOR_0") {
                    bufferColorSet0  = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    color0ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                }

                if (attrName == "JOINTS_0") {
                    // TODO load joints data
                }

                if (attrName == "WEIGHTS_0") {
                    // TODO load weights data
                }
            }

            for (size_t v = 0; v < vertexCount; v++) {
                Vertex &vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
                vert.pos     = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                vert.normal  = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                vert.uv0     = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                vert.uv1     = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
                vert.color   = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);

                // TODO joint and weight
                vert.joint0  = glm::vec4(0.0f);
                vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);

                // Fix for all zero weights
                if (glm::length(vert.weight0) == 0.0f) {
                    vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                }
                loaderInfo.vertexPos++;
            }

            if (hasIndices) {
                const tinygltf::Accessor   &accessor   = gltfModel.accessors[gltfPrimitive.indices > -1 ? gltfPrimitive.indices : 0];
                const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer     &buffer     = gltfModel.buffers[bufferView.buffer];

                indexCount          = static_cast<uint32_t>(accessor.count);
                const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t *buf = static_cast<const uint32_t *>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
                        loaderInfo.indexPos++;
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t *buf = static_cast<const uint16_t *>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
                        loaderInfo.indexPos++;
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t *buf = static_cast<const uint8_t *>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
                        loaderInfo.indexPos++;
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }

            Primitive *primitive = new Primitive(indexStart, indexCount, vertexCount, gltfPrimitive.material > -1 ? &materials[gltfPrimitive.material] : &materials.back());
            primitive->mode      = convertPrimitiveMode(gltfPrimitive.mode);
            primitive->setBoundingBox(posMin, posMax);

            newMesh->primitives.push_back(primitive);
            // Mesh BB from BBs of primitives
            for (auto *p : newMesh->primitives) {
                if (p->bb.valid && !newMesh->bb.valid) {
                    newMesh->bb       = p->bb;
                    newMesh->bb.valid = true;
                }
                newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
                newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
            }
        }
        newNode->mesh = newMesh;
    }

    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (gltfNode.translation.size() == 3) {
        translation          = glm::make_vec3(gltfNode.translation.data());
        newNode->translation = translation;
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (gltfNode.rotation.size() == 4) {
        glm::quat q       = glm::make_quat(gltfNode.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (gltfNode.scale.size() == 3) {
        scale          = glm::make_vec3(gltfNode.scale.data());
        newNode->scale = scale;
    }
    if (gltfNode.matrix.size() == 16) {
        newNode->matrix = glm::make_mat4x4(gltfNode.matrix.data());
    };

    // Node with children
    if (!gltfNode.children.empty()) {
        for (int n : gltfNode.children) {
            loadNode(newNode, gltfModel.nodes[n], n, gltfModel, loaderInfo, globalscale);
        }
    }

    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
}
void EntityLoader::getNodeProps(const tinygltf::Node &node, const tinygltf::Model &model, size_t &vertexCount, size_t &indexCount) {
    if (!node.children.empty()) {
        for (int n : node.children) {
            getNodeProps(model.nodes[n], model, vertexCount, indexCount);
        }
    }
    if (node.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        for (auto primitive : mesh.primitives) {
            vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
            if (primitive.indices > -1) {
                indexCount += model.accessors[primitive.indices].count;
            }
        }
    }
}

EntityLoader::Node *EntityLoader::findNode(Node *parent, uint32_t index) {
    Node *nodeFound = nullptr;
    if (parent->index == index) {
        return parent;
    }
    for (auto &child : parent->children) {
        nodeFound = findNode(child, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}
EntityLoader::Node *EntityLoader::getNodeFromIndex(uint32_t index) {
    Node *nodeFound = nullptr;
    for (auto &node : nodes) {
        nodeFound = findNode(node, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

} // namespace vklt
