//
// Created by npchitman on 6/26/21.
//

#include "Scene.h"

#include <Aphrodite/Physics/Physics2D.h>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Aphrodite/Renderer/Renderer.h"
#include "Aphrodite/Renderer/Renderer2D.h"
#include "Aphrodite/Renderer/SceneRenderer.h"
#include "Components.h"
#include "Entity.h"
#include "pch.h"

namespace Aph {

    Scene::Scene() {
        SceneRenderer::Init();
    }

    Scene::~Scene() = default;

    template<typename T>
    static void CopyComponent(entt::registry& destRegistry, entt::registry& srcRegistry, const std::unordered_map<uint32_t, entt::entity>& entityMap) {
        auto components = srcRegistry.view<T>();
        for (auto srcEntity : components) {
            auto destEntity = entityMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

            auto& srcComponent = srcRegistry.get<T>(srcEntity);
            auto& destComponent = destRegistry.emplace<T>(destEntity, srcComponent);
        }
    }

    void Scene::CopyTo(Ref<Scene>& target) {
        std::unordered_map<uint32_t, entt::entity> enttMap;
        auto idComponents = m_Registry.view<IDComponent>();
        for (auto entity : idComponents) {
            auto uuid = m_Registry.get<IDComponent>(entity).ID;
            auto e = target->CreateEntityWithID(uuid);
            enttMap[uuid] = static_cast<entt::entity>(e);
        }

        CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<MeshComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<LightComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<NativeScriptComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<Rigidbody2DComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<SkylightComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
        CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
    }

    Entity Scene::CreateEntity(const std::string& name) {
        Entity entity{m_Registry.create(), this};

        // default id component
        auto id = static_cast<uint32_t>(entity);
        //        APH_CORE_INFO("Add component, id: {}", id);
        auto& idComponent = entity.AddComponent<IDComponent>(id);

        // default transform component
        entity.AddComponent<TransformComponent>();

        // default tag component
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[idComponent.ID] = static_cast<entt::entity>(entity);

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

    bool Scene::HasEntity(Entity entity) {
        return HasEntity(static_cast<uint32_t>(entity));
    }

    bool Scene::HasEntity(uint32_t entityID) {
        return m_EntityMap.find(entityID) != m_EntityMap.end();
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


    void Scene::OnRuntimeUpdate(Timestep ts) {
        // Update Scripts
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
        glm::vec3 cameraPosition;
        {
            auto view = m_Registry.view<TransformComponent, CameraComponent>();
            for (auto entity : view) {
                auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

                if (camera.Primary) {
                    mainCamera = &camera.Camera;
                    cameraTransform = transform.GetTransform();
                    cameraPosition = transform.Translation;
                    break;
                }
            }
        }

        if (mainCamera) {

            // 3D ==============================================================
            Renderer::BeginScene(*mainCamera, cameraTransform);
            Ref<TextureCube> textureCube;

            {
                auto view = m_Registry.view<SkylightComponent>();
                for (auto entity : view) {
                    auto skylight = view.get<SkylightComponent>(entity);
                    if (skylight.Texture)
                        textureCube = skylight.Texture;
                }
            }

            if (textureCube)
                Renderer::DrawSkybox(textureCube, *mainCamera, cameraTransform);

            Renderer::EndScene();
            // ================================================================

            std::vector<Entity> lights;
            auto view = m_Registry.view<IDComponent, TransformComponent, LightComponent>();
            for (auto entity : view) {
                auto idComponent = view.get<IDComponent>(entity);
                lights.emplace_back(m_EntityMap[idComponent.ID], this);
            }

            SceneRenderer::BeginScene(*mainCamera,
                                      cameraTransform,
                                      cameraPosition,
                                      lights);

            // Bind irradiance map before mesh drawing.
            if (textureCube)
                textureCube->Bind(1);

            {
                auto view = m_Registry.view<TransformComponent, MeshComponent>();
                for (auto entity : view) {
                    auto [transform, mesh] = view.get<TransformComponent, MeshComponent>(entity);
                    if (mesh.mesh) {
                        SceneRenderer::SubmitMesh(mesh.mesh, transform.GetTransform());
                    }
                }
            }

            SceneRenderer::EndScene();
            // ================================================================

            // 2D==============================================================
            Renderer2D::BeginScene(*mainCamera, cameraTransform);
            {
                auto view = m_Registry.view<TransformComponent, Rigidbody2DComponent>();
                for (auto entity : view) {
                    auto [transform, rigidbody2d] = view.get<TransformComponent, Rigidbody2DComponent>(entity);

                    rigidbody2d.Body2D->SetTransform(transform.Translation, transform.Rotation.z);
                }

                Physics2D::OnUpdate();

                for (auto entity : view) {
                    auto [transform, rigidbody2d] = view.get<TransformComponent, Rigidbody2DComponent>(entity);

                    glm::vec2 position = rigidbody2d.Body2D->GetPosition();
                    transform.Translation = glm::vec3(position.x, position.y, transform.Translation.z);

                    transform.Rotation = glm::vec3(transform.Rotation.x, transform.Rotation.y, rigidbody2d.Body2D->GetRotation());
                }
            }

            {
                auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
                for (auto entity : view) {
                    auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                    Renderer2D::DrawQuad(static_cast<uint32_t>(entity), transform.GetTransform(),
                                         sprite.Texture, sprite.Color, sprite.TilingFactor);
                }

                Renderer2D::EndScene();
                // ================================================================
            }
        }
    }

    void Scene::OnEditorUpdate(Timestep ts, EditorCamera& camera) {
        // 3D==============================================================
        // Render Skylight
        Renderer::BeginScene(camera);

        Ref<TextureCube> textureCube;
        {
            auto view = m_Registry.view<SkylightComponent>();
            for (auto entity : view) {
                auto skylight = view.get<SkylightComponent>(entity);
                if (skylight.Texture)
                    textureCube = skylight.Texture;
            }
        }

        if (textureCube)
            Renderer::DrawSkybox(textureCube, camera);

        Renderer::EndScene();

        std::vector<Entity> lights;
        auto view = m_Registry.view<IDComponent, TransformComponent, LightComponent>();
        for (auto entity : view) {
            auto idComponent = view.get<IDComponent>(entity);
            lights.emplace_back(m_EntityMap[idComponent.ID], this);
        }

        SceneRenderer::BeginScene(camera, lights);

        // Bind irradiance map before mesh drawing.
        if (textureCube)
            textureCube->Bind(1);
        {
            auto view = m_Registry.view<TransformComponent, MeshComponent>();
            for (auto entity : view) {
                auto [transform, mesh] = view.get<TransformComponent, MeshComponent>(entity);
                if (mesh.mesh) {
                    SceneRenderer::SubmitMesh(mesh.mesh, transform.GetTransform());
                }
            }
        }

        SceneRenderer::EndScene();
        // ================================================================


        // 2D==============================================================
        Renderer2D::BeginScene(camera);
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entity : view) {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
                Renderer2D::DrawQuad(static_cast<int>(entity), transform.GetTransform(), sprite.Texture, sprite.Color, sprite.TilingFactor);
            }
        }
        Renderer2D::EndScene();
        // ================================================================
    }

    void Scene::OnRuntimePause(Timestep ts) {
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
        Physics2D::Init();

        {
            auto view = m_Registry.view<TransformComponent, Rigidbody2DComponent>();
            for (auto entity : view) {
                auto [transform, rigidbody2d] = view.get<TransformComponent, Rigidbody2DComponent>(entity);
                rigidbody2d.StartSimulation(transform.Translation, transform.Rotation.z);
            }
        }

        {
            auto view = m_Registry.view<TransformComponent, BoxCollider2DComponent>();
            for (auto entity : view) {
                auto [transform, boxCollider2D] = view.get<TransformComponent, BoxCollider2DComponent>(entity);
                boxCollider2D.Scale = transform.Scale;
            }
        }

        {
            auto view = m_Registry.view<Rigidbody2DComponent, BoxCollider2DComponent>();
            for (auto entity : view) {
                auto [rigidbody2d, boxCollider2d] = view.get<Rigidbody2DComponent, BoxCollider2DComponent>(entity);
                boxCollider2d.StartSimulation(rigidbody2d.Body2D);
            }
        }

        {
            auto view = m_Registry.view<Rigidbody2DComponent, CircleCollider2DComponent>();
            for (auto entity : view) {
                auto [rigidbody2d, circleCollider2d] = view.get<Rigidbody2DComponent, CircleCollider2DComponent>(entity);
                circleCollider2d.StartSimulation(rigidbody2d.Body2D);
            }
        }
    }

    void Scene::OnRuntimeEnd() {
        // TODO: runtime destroy
    }


    ////////////////////////////////////////////////
    // OnComponentAdd specialization ///////////////
    ////////////////////////////////////////////////

    template<typename T>
    void Scene::OnComponentAdded(Entity entity, T& component) {
        APH_ASSERT(false);
    }

    template<>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component) {
    }
    template<>
    void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component) {
//        component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
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

    template<>
    void Scene::OnComponentAdded<SkylightComponent>(Entity entity, SkylightComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<LightComponent>(Entity entity, LightComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent& component) {
    }

    template<>
    void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component) {
    }
}// namespace Aph