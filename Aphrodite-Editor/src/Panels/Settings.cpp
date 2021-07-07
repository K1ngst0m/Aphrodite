//
// Created by npchitman on 7/7/21.
//

#include "Settings.h"

#include <Aphrodite/Physics/Physics2D.h>

#include <glm/gtc/type_ptr.hpp>

namespace Aph::Editor {

    void Settings::OnImGuiRender() {


        ImGui::Begin("Settings");

        ImGui::Text("2D Gravity");
        ImGui::SameLine();
        ImGui::DragFloat2("##Gravity2D", glm::value_ptr(Physics2D::Gravity), 0.1f);

        ImGui::Text("2D Physics Timestep");
        ImGui::SameLine();
        ImGui::DragFloat("##2DPhysicsTimestep", &Physics2D::Timestep, 0.001f, 0.0001f, 0, "%.4f");

        ImGui::Text("Velocity Iterations");
        ImGui::SameLine();
        ImGui::DragInt("##VelocityIterations", &Physics2D::VelocityIterations, 1, 0);

        ImGui::Text("Position Iterations");
        ImGui::SameLine();
        ImGui::DragInt("##PositionIterations", &Physics2D::PositionIterations, 1, 0);
        ImGui::End();

    }
}// namespace Aph::Editor