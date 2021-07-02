//
// Created by npchitman on 7/2/21.
//

#include "ImGuiConsole.h"

#include <imgui.h>

#include "pch.h"

namespace Aph {
    uint32_t ImGuiConsole::s_LogMessageCount = 0;
    std::vector<Message> ImGuiConsole::s_MessageBuffer(0);

    static bool s_ShowInfoMessages = true;
    static bool s_ShowWarnMessages = true;
    static bool s_ShowErrorMessages = true;

    void ImGuiConsole::Draw() {
        if (ImGui::Button("Clear", {50.0f, 35.0f})) {
            Clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Log", {50.0f, 35.0f})) {
            s_ShowInfoMessages = !s_ShowInfoMessages;
        }

        ImGui::SameLine();
        if (ImGui::Button("Warn", {50.0f, 35.0f})) {
            s_ShowWarnMessages = !s_ShowWarnMessages;
        }

        ImGui::SameLine();
        if (ImGui::Button("Error", {50.0f, 35.0f})) {
            s_ShowErrorMessages = !s_ShowErrorMessages;
        }

        ImGui::Separator();
        ImGui::Separator();

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("Scrolling Region", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (uint32_t i = 0; i < ImGuiConsole::s_LogMessageCount; i++) {
            if (s_MessageBuffer.at(i).m_MessageLevel == Message::Level::Info) {
                ImGui::TextColored({1.0f, 1.0f, 1.0f, 1.0f}, "%s", s_MessageBuffer.at(i).m_MessageData.c_str());
                ImGui::Separator();
                continue;
            }
            if (s_MessageBuffer.at(i).m_MessageLevel == Message::Level::Warn) {
                ImGui::TextColored({1.0f, 1.0f, 0.0f, 1.0f}, "%s", s_MessageBuffer.at(i).m_MessageData.c_str());
                ImGui::Separator();
                continue;
            }
            if (s_MessageBuffer.at(i).m_MessageLevel == Message::Level::Error) {
                ImGui::TextColored({1.0f, 0.0f, 0.0f, 1.0f}, "%s", s_MessageBuffer.at(i).m_MessageData.c_str());
                ImGui::Separator();
                continue;
            }
        }
        ImGui::EndChild();
    }

    void ImGuiConsole::Clear() {
        s_LogMessageCount = 0;
        s_MessageBuffer.clear();
    }

}// namespace Aph
