//
// Created by npchitman on 6/27/21.
//

#ifndef Aphrodite_ENGINE_COMPONENTS_H
#define Aphrodite_ENGINE_COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <Aphrodite/Physics/BoxCollider2D.h>
#include <Aphrodite/Physics/Rigidbody2D.h>

#include <glm/gtx/quaternion.hpp>

#include "Aphrodite/Renderer/Texture.h"
#include "Aphrodite/Scene/SceneCamera.h"
#include "Aphrodite/Scene/ScriptableEntity.h"

namespace Aph {
    struct IDComponent {
        uint32_t ID = 0;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        explicit IDComponent(const uint32_t id)
            : ID(id) {}
        explicit operator uint32_t() const { return ID; }
    };

    struct TagComponent {
        std::string Tag;
        bool renaming = false;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        explicit TagComponent(std::string tag) : Tag(std::move(tag)) {}
    };

    struct TransformComponent {
        glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        explicit TransformComponent(const glm::vec3& translation) : Translation(translation) {}

        glm::mat4 GetTransform() const {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

            return translation * rotation * scale;
        }
    };

    struct Rigidbody2DComponent {
        Rigidbody2D::Rigidbody2DSpecification Specification;
        Ref<Rigidbody2D> Body2D;

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent&) = default;

        void StartSimulation(const glm::vec2& translation, const float rotation) {
            Body2D = CreateRef<Rigidbody2D>(translation, rotation, Specification);
        }

        void ValidateSpecification() {
            if (Body2D)
                Body2D->SetSpecification(Specification);
        }
    };

    struct BoxCollider2DComponent {
        glm::vec2 Size{1.0f, 1.0f};
        glm::vec2 Offset{0.0f, 0.0f};
        bool IsTrigger = false;

        Ref<BoxCollider2D> Collider2D;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent&) = default;

        void StartSimulation(Ref<Rigidbody2D>& rigidbody2D) {
            Collider2D = CreateRef<BoxCollider2D>(rigidbody2D, Size, Offset, IsTrigger);
        }

        void ValidateSpecification() {
            if (Collider2D)
                Collider2D->SetSpecification(Size, Offset, IsTrigger);
        }
    };

    struct SpriteRendererComponent {
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        Ref<Texture2D> Texture = nullptr;
        float TilingFactor = 1.0f;
        std::string TextureFilepath;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        explicit SpriteRendererComponent(const glm::vec4& color)
            : Color(color) {}
        void SetTexture(std::string& filepath) {
            Texture = Texture2D::Create(filepath);
            TextureFilepath = filepath;
        }

        void RemoveTexture() {
            Texture = nullptr;
            TextureFilepath = "";
        }
    };

    struct CameraComponent {
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    struct NativeScriptComponent {
        ScriptableEntity* Instance = nullptr;

        ScriptableEntity* (*InstantiateScript)(){};
        void (*DestroyScript)(NativeScriptComponent*){};

        template<typename T>
        void Bind() {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
        }
    };

    struct SkylightComponent {
        Ref<TextureCube> Texture = nullptr;
        std::string TextureFilepath;

        SkylightComponent() = default;
        SkylightComponent(const SkylightComponent&) = default;
        void SetTexture(std::string& filepath) {
            Texture = TextureCube::Create(filepath);
            TextureFilepath = filepath;
        }
        void RemoveTexture() {
            Texture = nullptr;
            TextureFilepath = "";
        }
    };
}// namespace Aph


#endif//Aphrodite_ENGINE_COMPONENTS_H
