//
// Created by npchitman on 6/26/21.
//

#ifndef HAZEL_ENGINE_SCENE_H
#define HAZEL_ENGINE_SCENE_H

#include "Hazel/Core/TimeStep.h"
#include <entt.hpp>

namespace Hazel {
    class Scene {
    public:
        Scene();
        ~Scene();

        entt::entity CreateEntity();

        entt::registry& Reg() { return m_Registry; }

        void OnUpdate(Timestep ts);
    private:
        entt::registry m_Registry;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_SCENE_H
