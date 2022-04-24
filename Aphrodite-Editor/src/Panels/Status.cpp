//
// Created by npchitman on 7/8/21.
//

#include "Status.h"

#include <imgui.h>

#include <Aphrodite.h>

#include "../EditorLayer.h"
#include "../Utils/UIDrawer.h"
#include "EditorConsole.h"

namespace Aph::Editor {

    void Status::OnUIRender() {
        DrawStatusBar();
        DrawStatusPanel();
    }

    void Status::DrawStatusBar() {
        static float sum = 0.0f;

        const auto size = m_FrameTimes.size();
        if (size >= 50){
            sum -= m_FrameTimes.front();
            m_FrameTimes.erase(m_FrameTimes.begin());
        }

        m_FrameTimes.push_back(ImGui::GetIO().Framerate);
        sum += m_FrameTimes.back();
        float avg = sum / static_cast<float>(size);

        UIDrawer::Draw([&]() {
                         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 4));
                         ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));

                         ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                         ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                         ImGui::PushStyleColor(ImGuiCol_Text, {1, 1, 1, 1});

                         ImGui::Begin("Status Bar", nullptr, ImGuiWindowFlags_NoScrollbar); },
                       [&]() {
                           ImGui::End();
                           ImGui::PopStyleColor(3);
                           ImGui::PopStyleVar(2);
                       },
                       [&]() {
                           // Dock Property
                           ImGui::Columns(3, "Status Bar", true);

                           ImGui::SetColumnWidth(0, 1000);// Console
                           ImGui::SetColumnWidth(1, 350); // Hovered Entity
                           ImGui::SetColumnWidth(2, 600); // FPS

                           // Console
                           ImGui::Text("%s", EditorConsole::GetLastMessage().data());
                           ImGui::NextColumn();

                           // Mouse Hover Entity
                           ImGui::Text(" \uf1b2 Hovered Entity: %s", EditorLayer::GetHoveredComponentName().c_str());
                           ImGui::NextColumn();

                           // Frame Per Second
                           ImGui::Text("FPS: %.2f", avg);
                           ImGui::SameLine(0, 70);
                           //                         const float fps = (1.0f / avg) * 1000.0f;
                           //                         ImGui::Text("Frame time (ms): %f", fps);
                           ImGui::PlotLines("", m_FrameTimes.data(), size);
                       });
    }

    void Status::DrawStatusPanel() {
        // Render StatusData
        ImGui::Begin(Style::Title::Renderer2DStatistics.data());
        auto stats = Renderer2D::GetStats();
        ImGui::Text("# Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("# Quads: %d", stats.QuadCount);
        ImGui::Text("# Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("# Indices: %d", stats.GetTotalIndexCount());
        ImGui::End();

        // Renderer Info
        ImGui::Begin(Style::Title::RenderInfo.data());
        ImGui::Text("# Vendor         : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Vendor);
        ImGui::Text("# Hardware       : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Renderer);
        ImGui::Text("# OpenGL Version : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Version);
        ImGui::End();
    }
}// namespace Aph::Editor
