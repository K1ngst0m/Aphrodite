#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "vklCamera.h"
#include "vklObject.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

namespace vkl {

enum class SCENE_UNIFORM_TYPE : uint8_t {
    UNDEFINED,
    CAMERA,
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    FLASH_LIGHT,
};

enum class SCENE_RENDER_TYPE : uint8_t {
    OPAQUE,
    TRANSPARENCY,
};

struct SceneNode {
    SceneNode *_parent;
    Object    *_object;
    glm::mat4  _transform;

    std::vector<SceneNode *> _children;

    SceneNode *createChildNode() {
        SceneNode *childNode = new SceneNode;
        _children.push_back(childNode);
        return childNode;
    }

    void attachObject(Object *object) {
        _object = object;
    }
};

struct SceneRenderNode : SceneNode {
    vkl::RenderObject *_object;
    vkl::ShaderPass   *_pass;

    SceneRenderNode(vkl::RenderObject *object, vkl::ShaderPass *pass, glm::mat4 transform)
        : _object(object), _pass(pass) {
        _transform = transform;
    }
};

struct SceneUniformNode : SceneNode {
    SCENE_UNIFORM_TYPE _type;

    vkl::UniformBufferObject *_object = nullptr;

    SceneUniformNode(vkl::UniformBufferObject *object, SCENE_UNIFORM_TYPE uniformType = SCENE_UNIFORM_TYPE::UNDEFINED)
        : _type(uniformType), _object(object) {
    }
};

struct SceneCameraNode : SceneUniformNode {
    SceneCameraNode(vkl::UniformBufferObject *object, vkl::Camera *camera)
        : SceneUniformNode(object, SCENE_UNIFORM_TYPE::CAMERA), _camera(camera) {
    }

    vkl::Camera *_camera;
};

class Scene {
public:
    Scene() {
        rootNode = new SceneNode;
    }
    Scene &pushUniform(UniformBufferObject *ubo);
    Scene &pushCamera(vkl::Camera *camera, UniformBufferObject *ubo);
    Scene &pushMeshObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f), SCENE_RENDER_TYPE renderType = SCENE_RENDER_TYPE::OPAQUE);

    uint32_t getRenderableCount() const {
        return _renderNodeList.size();
    }
    uint32_t getUBOCount() const {
        return _uniformNodeList.size() + _cameraNodeList.size();
    }

private:
    SceneNode *rootNode;

public:
    std::vector<SceneRenderNode *>  _renderNodeList;
    std::vector<SceneUniformNode *> _uniformNodeList;
    std::vector<SceneCameraNode *>  _cameraNodeList;
};

class GlTFScene : public Scene {
    enum class AccessorType {
        SCALAR,
        VEC2,
        VEC3,
        VEC4,
        MAT2,
        MAT3,
        MAT4,
    };

    enum class AccessorDataType {
        SIGNED_BYTE    = 5120,
        UNSIGNED_BYTE  = 5121,
        SIGNED_SHORT   = 5122,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT   = 5125,
        FLOAT          = 5126,
    };

    /**
     All binary data for meshes, skins, and animations
     is stored in buffers and retrieved via accessors.
    */
    struct Accessor {
        uint32_t     bufferView;
        uint32_t     byteOffset;
        uint32_t     componentType;
        uint32_t     count;
        AccessorType type;

        // accessors bounds
        std::vector<uint32_t> max;
        std::vector<uint32_t> min;
        // TODO sparse accessors
    };

    struct Node {
        std::string name;

        glm::vec4 rotation;
        glm::vec3 scale;
        glm::vec3 translation;
        glm::mat4 matrix;

        uint32_t mesh;

        std::vector<Node *> children;
    };

    struct Buffer {
        std::string uri;
        uint32_t    byteLength;
    };

    struct BufferView {
        uint32_t buffer;
        uint32_t byteLength;
        uint32_t byteOffset;

        // ???
        uint32_t target;
        // used for vertex attribute data
        uint32_t byteStride;
    };

    struct Texture {
        uint32_t source;
        uint32_t sampler;
    };

    struct Sampler{
        uint32_t magFilter;
        uint32_t minFilter;
        uint32_t wrapS;
        uint32_t wrapT;
    };

    enum class ImageMimeType {
        PNG,
        JPG,
    };

    struct Image {
        std::string   uri;
        uint32_t      bufferView;
        ImageMimeType type;
    };

    struct Material {
        std::string name;

        struct {
            glm::vec4 baseColorFactor;
            glm::vec3 EmissiveFactor;
            float metallicFactor;
            float roughnessFactor;

            struct {
                uint32_t index;
                uint32_t scale;
                uint32_t texCoord;
            } baseColorTexture;

            struct{
                uint32_t index;
                uint32_t scale;
                uint32_t texCoord;
            } metallicRoughnessTexture;

        } pbrMetallicRoughness;

        struct{
            uint32_t index;
            uint32_t scale;
            uint32_t texCoord;
        } normalTexture;
    };

    enum class VertexComponent {
        Position,
        Normal,
        UV,
        Color,
        Tangent,
        Joint0,
        Weight0
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 color;
        glm::vec4 joint0;
        glm::vec4 weight0;
        glm::vec4 tangent;
    };

    struct VertexAttribute{
        uint32_t NORMAL;
        uint32_t POSITION;
        uint32_t TANGENT;
        uint32_t TEXCOORD;
        uint32_t COLOR;
        uint32_t JOINTS;
        uint32_t WEIGHTS;
    };

    struct Primitive {
        // TODO load vertex attributes
        VertexAttribute attributes;
        uint32_t indices;
        uint32_t material;
        uint32_t mode;
    };

    struct Mesh {
        std::string            name;
        std::vector<Primitive> primitives;
    };

    enum class CameraType{
        PERSPECTIVE,
        ORTHOGRAPHIC,
    };

    struct Camera{
        std::string name;
        CameraType type;
        struct {
            float xmag;
            float ymag;
            float znear;
            float zfar;
        } orthographic;
        struct {
            float aspectRatio;
            float yfov;
            float znear;
            float zfar;
        } perspective;
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

    std::string path;

    std::vector<Node *>     nodes;
    std::vector<Buffer>     buffers;
    std::vector<BufferView> bufferViews;
    std::vector<Texture>    textures;
    std::vector<Sampler>    samplers;
    std::vector<Image>      images;
    std::vector<Accessor>   accessors;
    std::vector<Material>   materials;
    std::vector<Vertex>     vertices;
    std::vector<Camera>     cameras;

    void loadSceneFromFile(const std::string& filename, uint32_t fileLoadingFlags = FileLoadingFlags::None, float globalScale = 1.0f){
        tinygltf::Model    gltfModel;
        tinygltf::TinyGLTF gltfContext;
        if (fileLoadingFlags & FileLoadingFlags::DontLoadImages) {
            gltfContext.SetImageLoader([](tinygltf::Image *image, const int imageIndex, std::string *error, std::string *warning, int req_width, int req_height, const unsigned char *bytes, int size, void *userData) {
                return true;
            },
                                       nullptr);
        } else {
            gltfContext.SetImageLoader([](tinygltf::Image *image, const int imageIndex, std::string *error, std::string *warning, int req_width, int req_height, const unsigned char *bytes, int size, void *userData) {
                // handle ktx maunally
                if (image->uri.find_last_of('.') != std::string::npos &&
                    (image->uri.substr(image->uri.find_last_of('.') + 1) == "ktx")) {
                    return true;
                }

                return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
            }, nullptr);
        }

        size_t pos = filename.find_last_of('/');
        path = filename.substr(0, pos);

        std::string error, warning;

        bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        if (fileLoaded){
            if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages)) {
                loadImages(gltfModel);
            }
        }
        else{
            assert("Failed to load scene from gltf file.");
        }
    }

    void loadImages(const tinygltf::Scene& gltfScene){

    }

    void loadNodes(){

    }

    void loadBinaryData(){

    }

    void loadGeometryData(){

    }

    void loadTextureData(){

    }

    void loadMaterials(){

    }

    void loadCamera(){

    }

    static uint8_t convertAccessorType(AccessorType type) {
        switch (type) {
        case AccessorType::SCALAR:
            return 1;
        case AccessorType::VEC2:
            return 2;
        case AccessorType::VEC3:
            return 3;
        case AccessorType::VEC4:
        case AccessorType::MAT2:
            return 4;
        case AccessorType::MAT3:
            return 9;
        case AccessorType::MAT4:
            return 16;
        }
    }
    static uint32_t convertAccessorDataType(AccessorDataType type) {
        switch (type) {
        case AccessorDataType::SIGNED_BYTE:
        case AccessorDataType::UNSIGNED_BYTE:
            return 8;
        case AccessorDataType::SIGNED_SHORT:
        case AccessorDataType::UNSIGNED_SHORT:
            return 16;
        case AccessorDataType::UNSIGNED_INT:
        case AccessorDataType::FLOAT:
            return 32;
        }
    };
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
