//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_MATERIAL_H
#define APHRODITE_MATERIAL_H

namespace Aph {
    class TextureCube;
    class Texture2D;
    class Shader;

    class Material {
    public:
        virtual ~Material() = default;

        virtual void Bind() const = 0;
        std::string Name;
    };

    class MaterialInstance : public Material {
    public:
        ~MaterialInstance() override = default;

        enum Type { PBR = 0 };

        glm::vec4 Color = glm::vec4(1.0f);

        static Ref<MaterialInstance> Create(Type type);
        virtual Ref<Shader> GetShader() const { return m_Shader; }

    protected:
        Ref<Shader> m_Shader = nullptr;
    };

    class PbrMaterial : public MaterialInstance {
    public:
        PbrMaterial();
        ~PbrMaterial() override = default;

        void Bind() const override;

        float Metallic = 0.5f;
        float Roughness = 0.5f;
        float AO = 0.5f;

        glm::vec3 EmissiveColor = glm::vec3(0.0f, 0.0f, 0.0f);
        float EmissiveIntensity = 5.0f;

        bool UseAlbedoMap = false;
        bool UseMetallicMap = false;
        bool UseNormalMap = false;
        bool UseRoughnessMap = false;
        bool UseOcclusionMap = false;
        bool UseEmissiveMap = false;

        Ref<Texture2D> AlbedoMap;
        Ref<Texture2D> MetallicMap;
        Ref<Texture2D> NormalMap;
        Ref<Texture2D> RoughnessMap;
        Ref<Texture2D> AmbientOcclusionMap;
        Ref<Texture2D> OpacityMap;
        Ref<Texture2D> EmissiveMap;
    };

}// namespace Aph


#endif//APHRODITE_MATERIAL_H
