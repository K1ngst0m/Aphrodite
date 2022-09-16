#ifndef VKLGLTFSCENE_H_
#define VKLGLTFSCENE_H_

#include "vklSceneManger.h"

namespace vklt {

enum class PrimitiveMode{
    POINTS = 0,
    LINE = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6,
};

enum class CameraType{
    PERSPECTIVE,
    ORTHOGRAPHIC,
};

enum class FilterMode{
    UNDEFINED,
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR,
};

enum class SamplerAddressMode{
    REPEAT,
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT,
};

enum class PbrWorkflow{
    MetallicRoughness,
    SpecularGlossiness,
};

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
    bool valid = false;
    BoundingBox();
    BoundingBox(glm::vec3 min, glm::vec3 max);
    BoundingBox getAABB(glm::mat4 m);
};

class GlTFScene {
    struct Image {
        uint32_t       width;
        uint32_t       height;
        unsigned char *data;
        size_t         dataSize;
    };

    struct TextureSampler{
        FilterMode magFilter;
        FilterMode minFilter;
        SamplerAddressMode wrapS;
        SamplerAddressMode wrapT;
    };

    struct Texture {
        Image * image;
        TextureSampler sampler;
    };

    struct Material {
        enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(1.0f);
        bool doubleSided = false;

        Texture *baseColorTexture;
        Texture *metallicRoughnessTexture;
        Texture *normalTexture;
        Texture *occlusionTexture;
        Texture *emissiveTexture;

        struct {
            uint8_t baseColor = 0;
            uint8_t metallicRoughness = 0;
            uint8_t specularGlossiness = 0;
            uint8_t normal = 0;
            uint8_t occlusion = 0;
            uint8_t emissive = 0;
        } texCoordSets;

        PbrWorkflow pbrworkflow;

        std::string name;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 joint0;
        glm::vec4 weight0;
        glm::vec4 color;
    };

    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t vertexCount;
        Material* material;
        BoundingBox bb;
        bool hasIndices;
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material* material)
            : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
            hasIndices = indexCount > 0;
        };

        void setBoundingBox(glm::vec3 min, glm::vec3 max){
            bb.min = min;
            bb.max = max;
            bb.valid = true;
        }
        PrimitiveMode mode;
    };

    struct Mesh {
        std::string              name;
        BoundingBox              bb;
        std::vector<Primitive *> primitives;
    };

    struct Node {
        std::string name;
        uint32_t index;

        glm::quat rotation;
        glm::vec3 scale;
        glm::vec3 translation;
        glm::mat4 matrix;

        Mesh* mesh;

        uint32_t skinIndex;

        Node* parent;
        std::vector<Node *> children;
    };

    enum FileLoadingFlags {
        None = 0x00000000,
        PreTransformVertices = 0x00000001,
        PreMultiplyVertexColors = 0x00000002,
        FlipY = 0x00000004,
        DontLoadImages = 0x00000008
    };

    enum RenderFlags {
        BindImages = 0x00000001,
        RenderOpaqueNodes = 0x00000002,
        RenderAlphaMaskedNodes = 0x00000004,
        RenderAlphaBlendedNodes = 0x00000008
    };

    struct LoaderInfo {
        uint32_t* indexBuffer;
        Vertex* vertexBuffer;
        size_t indexPos = 0;
        size_t vertexPos = 0;
        size_t vertexCount;
        size_t indexCount;
    };

    void loadSceneFromFile(const std::string &filename,
                           uint32_t fileLoadingFlags = FileLoadingFlags::None,
                           float globalScale = 1.0f);

    Node *getNodeFromIndex(uint32_t index);

private:
    void loadMaterials(tinygltf::Model &gltfModel);
    void loadTextures(const tinygltf::Model &gltfModel);
    void loadImages(const tinygltf::Model &gltfModel);
    void loadNode(Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, LoaderInfo &loaderInfo, float globalscale);
    void getNodeProps(const tinygltf::Node &node, const tinygltf::Model &model, size_t &vertexCount, size_t &indexCount);
    Node *findNode(Node *parent, uint32_t index);

    // TODO load skins and animations
    void loadSkins(const tinygltf::Model &gltfModel){}
    void loadAnimations(const tinygltf::Model &gltfModel){}

private:
    std::vector<Node *>     nodes;
    std::vector<Texture>    textures;
    std::vector<Image>      images;
    std::vector<Material>   materials;
};

}

#endif // VKLGLTFSCENE_H_
