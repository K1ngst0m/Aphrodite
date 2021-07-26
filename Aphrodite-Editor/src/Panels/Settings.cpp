//
// Created by npchitman on 7/7/21.
//

#include "Settings.h"

#include <glm/gtc/type_ptr.hpp>

#include "../Utils/UIDrawer.h"

namespace Aph::Editor {

    void Settings::OnUIRender() {

        ImGui::Begin(Style::Title::Settings.data());

        ImGui::Text("Physics Property");

        ImGui::Separator();

        UIDrawer::DrawGrid("Gravity2D", [] {
            UIDrawer::Property("Gravity 2D", Physics2D::Gravity);
        });

        UIDrawer::DrawGrid("2DPhysicsTimestep", [] {
            UIDrawer::Property("2D Physics Timestep", Physics2D::Timestep, 0.1f, 0.001f, "%.4f");
        });

        UIDrawer::DrawGrid("VelocityIterations", [] {
            UIDrawer::Property("Velocity Iterations", Physics2D::VelocityIterations, 1, 50);
        });

        UIDrawer::DrawGrid("PositionIterations", [] {
            UIDrawer::Property("Position Iterations", Physics2D::PositionIterations, 1, 50);
        });

        ImGui::Text("Other");
        ImGui::Separator();

        ImGui::End();
    }
}// namespace Aph::Editor