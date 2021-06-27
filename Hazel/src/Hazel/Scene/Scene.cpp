//
// Created by npchitman on 6/26/21.
//

#include "Scene.h"

#include <glm/glm.hpp>

#include "Components.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "hzpch.h"

#include "Entity.h"

namespace Hazel {

    static void DoMath(const glm::mat4& transform) {
    }

    static void OnTransformConstruct(entt::registry& registry, entt::entity entity) {

    }

    Scene::Scene() { // NOLINT(modernize-use-equals-default)
#if ENTT_EXAMPLE_CODE
        auto entity = m_Registry.create();
        m_Registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));
        m_Registry.on_construct<TransformComponent>().connect<&OnTransformConstruct>();

        if (m_Registry.has<TransformComponent>(entity))
            TransformComponent& transform = m_Registry.get<TransformComponent>(entity);

        auto view = m_Registry.view<TransformComponent>();

        for (auto e : view) {
            auto& transform = view.get<TransformComponent>(entity);
        }

        auto group = m_Registry.group<TransformComponent>(entt::get<MeshComponent>);
        for (auto e : group) {
            auto& [transform, mesh] = group.get<TranformComponent, MeshComponent>(entity);
        }
#endif
    }

    Scene::~Scene() = default;

    Entity Scene::CreateEntity(const std::string& name) {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<TransformComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::OnUpdate(Timestep ts) {
        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
        for (auto entity : group) {
            const auto &[transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
            Renderer2D::DrawQuad(static_cast<const glm::mat4>(transform), sprite.Color);
        }
    }
}// namespace Hazel