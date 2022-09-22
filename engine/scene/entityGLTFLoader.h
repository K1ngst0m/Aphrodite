#ifndef ENTITYGLTFLOADER_H_
#define ENTITYGLTFLOADER_H_

#include "common.h"
#include "entity.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace vkl {
class EntityGLTFLoader : public EntityLoader{
public:
    EntityGLTFLoader(Entity *entity);
    void loadFromFile(const std::string &path) override;

private:
    void          loadImages(tinygltf::Model &input);
    void          loadMaterials(tinygltf::Model &input);
    void          loadNodes(const tinygltf::Node &inputNode, const tinygltf::Model &input, SubEntity *parent);
};
}

#endif // ENTITYGLTFLOADER_H_
