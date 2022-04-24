//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_MODEL_H
#define APHRODITE_MODEL_H

#include <assimp/scene.h>

#include <utility>
#include "Mesh.h"

namespace Aph {
    class MaterialInstance;
    class Timestep;
    class Texture2D;

    class Model {
    public:
        Model(int entityID, const std::string& filepath);
        ~Model() = default;

        std::vector<Mesh>& GetMeshes() { return m_Meshes; }
        Ref<MaterialInstance>& GetMaterialInstance(uint32_t index) { return m_Materials.at(index); }
        uint32_t GetMaterialsCount() const { return m_Materials.size(); }
        std::string& GetFilePath() { return m_Filepath; }
        std::string& GetName() { return m_Name; }

    private:
        void LoadModel(int entityID, const std::string& path);
        void ProcessNode(int entityID, aiNode* node, const aiScene* scene);
        Mesh LoadMesh(int entityID, aiMesh* mesh, const aiScene* scene);
        std::vector<Ref<Texture2D>> LoadMaterialTextures(aiMaterial* material,
                                                         aiTextureType type);

    private:
        std::vector<Mesh> m_Meshes;
        std::string m_Filepath;
        std::string m_Name;
        std::string m_Directory;
        std::vector<Ref<MaterialInstance>> m_Materials;
    };
}// namespace Aph


#endif//APHRODITE_MODEL_H
