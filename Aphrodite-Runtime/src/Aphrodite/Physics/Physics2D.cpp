//
// Created by npchitman on 7/7/21.
//

#include "Physics2D.h"

namespace Aph {

    // world properties
    glm::vec2 Physics2D::Gravity(0.0f, -9.8f);
    float Physics2D::Timestep = 0.02f;
    int Physics2D::VelocityIterations = 8;
    int Physics2D::PositionIterations = 3;

    Ref<b2World> Physics2D::s_World = nullptr;

    void Physics2D::Init() {
        s_World = CreateRef<b2World>(reinterpret_cast<b2Vec2&>(Gravity));
    }

    void Physics2D::OnUpdate() {
        s_World->Step(Timestep, static_cast<int>(VelocityIterations), static_cast<int>(PositionIterations));
    }
}// namespace Aph
