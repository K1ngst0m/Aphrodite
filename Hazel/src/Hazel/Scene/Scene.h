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

    private:
        entt::registry m_Registry;

        friend class Entity;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_SCENE_H
