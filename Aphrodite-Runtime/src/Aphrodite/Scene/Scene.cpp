//
// Created by npchitman on 6/26/21.
//

#include "Scene.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Aphrodite/Renderer/Renderer2D.h"
#include "Components.h"
#include "Entity.h"
#include "pch.h"

namespace Aph {

    Scene::Scene() = default;
    Scene::~Scene() = default;

    template<typename T>
    static void CopyComponent(entt::registry& destRegistry, entt::registry& srcRegistry, const std::unordered_map<uint32_t, entt::entity>& entityMap) {
        auto components = srcRegistry.view<T>();
        for (auto srcEntity : components) {
            entt::entity destEntity = entityMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

            auto& srcComponent = srcRegistry.get<T>(srcEntity);
            auto& destComponent = destRegistry.emplace<T>(destEntity, srcComponent);
        }
    }

    void Scene::CopyTo(Ref<Scene>& target) {
        std::unordered_map<uint32_t, entt::entity> enttMap;
        auto idComponents = m_Registry.view<IDComponent>();
        for (auto entity : idComponents) {
            auto uuid = m_Registry.get<IDComponent>(entity).ID;
            Entity e = target->CreateEntityWithID(uuid);
            enttMap[uuid] = static_cast<entt::entity>(e);
        }

        CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<NativeScriptComponent>(target->m_Registry, m_Registry, enttMap);
    }

    Entity Scene::CreateEntity(const std::string& name) {
        Entity entity = {m_Registry.create(), this};

        // default id component
        auto id = static_cast<uint32_t>(entity);
        auto& idComponent = entity.AddComponent<IDComponent>(id);

        // default transform component
        entity.AddComponent<TransformComponent>();

        // default tag component
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        return entity;
    }

    Entity Scene::CreateEntityWithID(const uint32_t id) {
        Entity entity = {m_Registry.create(), this};
        auto& idComponent = entity.AddComponent<IDComponent>(id);

        m_EntityMap[idComponent.ID] = static_cast<entt::entity>(entity);

        return entity;
    }

    void Scene::DestroyEntity(Entity entity) {
        m_Registry.destroy(static_cast<entt::entity>(entity));
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height) {
        m_ViewportHeight = height;
        m_ViewportWidth = width;

        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view) {
            auto& cameraComponent = view.get<CameraComponent>(entity);
            if (!cameraComponent.FixedAspectRatio)
                cameraComponent.Camera.SetViewportSize(width, height);
        }
    }

    template<typename T>
    void Scene::OnComponentAdded(Entity entity, T& component) {
        APH_ASSERT(false);
    }

    void Scene::OnUpdateRuntime(Timestep ts) {
        {
            m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc) {
                if (!nsc.Instance) {
                    nsc.Instance = nsc.InstantiateScript();
                    nsc.Instance->m_Entity = Entity{entity, this};

                    nsc.Instance->OnCreate();
                }

                nsc.Instance->OnUpdate(ts);
            });
        }

        // Renderer
        Camera* mainCamera = nullptr;
        glm::mat4 cameraTransform;
        {
            auto view = m_Registry.view<TransformComponent, CameraComponent>();
            for (auto entity : view) {
                auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

                if (camera.Primary) {
                    mainCamera = &camera.Camera;
                    cameraTransform = transform.GetTransform();
                    break;
                }
            }
        }

        if (mainCamera) {
            Renderer2D::BeginScene(*mainCamera, cameraTransform);

            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entity : view) {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(static_cast<uint32_t>(entity), transform.GetTransform(),
                                     sprite.Texture, sprite.Color, sprite.TilingFactor);
            }

            Renderer2D::EndScene();
        }
    }

    void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera) {
        Renderer2D::BeginScene(camera);

        auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
        for (auto entity : view) {
            auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
            Renderer2D::DrawQuad(static_cast<int>(entity), transform.GetTransform(), sprite.Texture, sprite.Color, sprite.TilingFactor);
        }

        Renderer2D::EndScene();
    }

    int Scene::GetPixelDataAtPoint(const int x, const int y) {
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        int pixelData;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        return pixelData;
    }

    Entity Scene::GetPrimaryCameraEntity() {
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view) {
            const auto& camera = view.get<CameraComponent>(entity);
            if (camera.Primary)
                return Entity(entity, this);
        }
        return {};
    }

    void Scene::OnRuntimeStart() {
        // TODO: runtime init
    }

    void Scene::OnRuntimeEnd() {
        // TODO: runtime destroy
    }

    template<>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
    {
    }

    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component) {
        component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
            component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
    }

    template<>
    void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component) {
    }
}// namespace Aph