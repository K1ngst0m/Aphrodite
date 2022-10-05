#ifndef ENTITYGLTFLOADER_H_
#define ENTITYGLTFLOADER_H_

#include "common/common.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
class Entity;
class SubEntity;

class ResourceManager{
public:
    void loadTextureFromFile(const std::string& path){
        bool fileLoaded = false;
    }

    void loadTextureFromManualData(void * data){

    }

    void loadBufferFromFile(const std::string &path){

    }

    void loadBufferFromMaunalData(void * data){

    }

private:
};

class GLTFLoader {
public:
    static void load(Entity *entity, const std::string& path);
private:
    static void _loadTextures(Entity* _entity, tinygltf::Model &input);
    static void _loadMaterials(Entity* _entity, tinygltf::Model &input);
    static void _loadNodes(Entity* _entity, const tinygltf::Node &inputNode, const tinygltf::Model &input, SubEntity *parent);
};
} // namespace vkl

#endif // ENTITYGLTFLOADER_H_
