//
// Created by npchitman on 6/27/21.
//

#ifndef HAZEL_ENGINE_ENTITY_H
#define HAZEL_ENGINE_ENTITY_H

#include <entt.hpp>

#include "Scene.h"

namespace Hazel {
    class Entity {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);
        Entity(const Entity& other) = default;

        template<typename T, typename... Args>

        T& AddComponent(Args&&... args) {
            HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");

            T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            m_Scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template<typename T>
        T& GetComponent() {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");

            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        template<typename T>
        bool HasComponent() {
            return m_Scene->m_Registry.has<T>(m_EntityHandle);
        }

        template<typename T>
        void RemoveComponent() {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        explicit operator entt::entity() const { return m_EntityHandle; }

        explicit operator bool() const { return m_EntityHandle != entt::null; }

        explicit operator uint32_t() const { return static_cast<uint32_t>(m_EntityHandle); }

        bool operator==(const Entity& other) const {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const {
            return !(*this == other);
        }

    private:
        entt::entity m_EntityHandle{entt::null};
        Scene* m_Scene = nullptr;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_ENTITY_H