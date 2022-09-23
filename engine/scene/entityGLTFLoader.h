#ifndef ENTITYGLTFLOADER_H_
#define ENTITYGLTFLOADER_H_

#include "common.h"
#include "entity.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
class GLTFLoader {
public:
    static void load(Entity *entity, const std::string& path);
private:
    static void loadImages(Entity* _entity, tinygltf::Model &input);
    static void loadMaterials(Entity* _entity, tinygltf::Model &input);
    static void loadNodes(Entity* _entity, const tinygltf::Node &inputNode, const tinygltf::Model &input, SubEntity *parent);
};
} // namespace vkl

#endif // ENTITYGLTFLOADER_H_
