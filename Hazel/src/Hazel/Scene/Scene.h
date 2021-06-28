//
// Created by npchitman on 6/26/21.
//

#ifndef HAZEL_ENGINE_SCENE_H
#define HAZEL_ENGINE_SCENE_H

#include <entt.hpp>

#include "Hazel/Core/TimeStep.h"
#include "Hazel/Renderer/EditorCamera.h"


namespace Hazel {
    class Entity;

    class Scene {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name = std::string());
        void DestroyEntity(Entity entity);

        void OnUpdateRuntime(Timestep ts);
        void OnUpdateEditor(Timestep ts, EditorCamera& camera);

        void OnViewportResize(uint32_t width, uint32_t height);

        Entity GetPrimaryCameraEntity();

    private:
        template<typename T>
        void OnComponentAdded(Entity entity, T& component);

    private:
        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_SCENE_H
