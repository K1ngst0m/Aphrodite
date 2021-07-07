//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_PHYSICS2D_H
#define APHRODITE_PHYSICS2D_H

#include <box2d/b2_world.h>

#include <glm/glm.hpp>

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Core/TimeStep.h"

namespace Aph {
    class Physics2D {
    public:
        static void Init();

        static void OnUpdate();

        static const Ref<b2World> GetWorld() { return s_World; }

    public:
        static glm::vec2 Gravity;
        static float Timestep;
        static int VelocityIterations;
        static int PositionIterations;

    private:
        static Ref<b2World> s_World;
    };
}// namespace Aph


#endif//APHRODITE_PHYSICS2D_H
