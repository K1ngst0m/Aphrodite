//
// Created by npchitman on 6/26/21.
//

#ifndef HAZEL_ENGINE_SCENE_H
#define HAZEL_ENGINE_SCENE_H

#include <entt.hpp>

#include "Hazel/Core/TimeStep.h"


namespace Hazel {
    class Entity;

    class Scene {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name = std::string());

        void OnUpdate(Timestep ts);

        void OnViewportResize(uint32_t width, uint32_t height);
    private:
        entt::registry m_Registry;
        uint32_t  m_ViewportWidth = 0, m_ViewportHeight = 0;

        friend class Entity;
        friend class SceneHierarchyPanel;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_SCENE_H
