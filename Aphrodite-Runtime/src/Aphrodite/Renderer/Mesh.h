#ifndef MESH_H_
#define MESH_H_

#include "Aphrodite/Utils/AABB.h"

namespace Aph {
    class VertexArray;
    class Texture2D;

    struct Vertex {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normal;
        int ObjectID;
    };

    struct Mesh {
        Mesh(const std::vector<Vertex>& vertices, std::vector<uint32_t> indices, std::vector<Ref<Texture2D>> textures);

        uint32_t BaseVertex{};
        uint32_t BaseIndex{};
        uint32_t MaterialIndex{};
        uint32_t IndexCount{};

        glm::mat4 Transform{};

        std::string NodeName{}, MeshName{};

        Ref<VertexArray> meshVertexArray{};
        std::vector<Vertex> Vertices{};
        std::vector<uint32_t> Indices{};
        std::vector<Ref<Texture2D>> Textures{};
        AABB BoundingBox{};
    };
}



#endif // MESH_H_
