//
// Created by npchitman on 6/26/21.
//

#ifndef Aphrodite_ENGINE_SCENE_H
#define Aphrodite_ENGINE_SCENE_H

#include <entt.hpp>

#include "Aphrodite/Core/TimeStep.h"
#include "Aphrodite/Renderer/EditorCamera.h"

namespace Aph {
    class Entity;

    class Scene {
    public:
        Scene();
        ~Scene();


        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithID(uint32_t id);
        void DestroyEntity(Entity entity);
        void CopyTo(Ref<Scene>& target);

        // runtime
        void OnRuntimeStart();
        void OnRuntimeEnd();
        void OnUpdateRuntime(Timestep ts);

        void OnUpdateEditor(Timestep ts, EditorCamera& camera);
        void OnViewportResize(uint32_t width, uint32_t height);

        Entity GetPrimaryCameraEntity();
        static int GetPixelDataAtPoint(const int x, const int y);

    private:
        template<typename T>
        void OnComponentAdded(Entity entity, T& component);

    private:
        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;

    private:
        std::unordered_map<uint32_t, entt::entity> m_EntityMap;
    };
}// namespace Aph


#endif//Aphrodite_ENGINE_SCENE_H
