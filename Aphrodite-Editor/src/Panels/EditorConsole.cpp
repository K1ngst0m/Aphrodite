//
// Created by npchitman on 7/2/21.
//

#include "EditorConsole.h"

#include <imgui.h>

#include "pch.h"

namespace Aph::Editor {
    uint32_t EditorConsole::s_LogMessageCount = 0;
    std::vector<Message> EditorConsole::s_MessageBuffer(0);

    static bool s_ShowInfoMessages = true;
    static bool s_ShowWarnMessages = true;
    static bool s_ShowErrorMessages = true;


    void EditorConsole::Draw() {
        ImGui::Begin(Style::Title::Console.data());
        const ImVec2 consoleButtonSize = {65.0f, 35.0f};

        if (ImGui::Button("Clear", consoleButtonSize)) {
            Clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Log", consoleButtonSize)) {
            s_ShowInfoMessages = !s_ShowInfoMessages;
        }

        ImGui::SameLine();
        if (ImGui::Button("Warn", consoleButtonSize)) {
            s_ShowWarnMessages = !s_ShowWarnMessages;
        }

        ImGui::SameLine();
        if (ImGui::Button("Error", consoleButtonSize)) {
            s_ShowErrorMessages = !s_ShowErrorMessages;
        }

        ImGui::Separator();

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("Scrolling Region", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto & message : s_MessageBuffer) {
            if (message.m_MessageLevel == Message::Level::Info && s_ShowInfoMessages)
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "%s", message.m_MessageData.c_str());
            if (message.m_MessageLevel == Message::Level::Warn && s_ShowWarnMessages)
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "%s", message.m_MessageData.c_str());
            if (message.m_MessageLevel == Message::Level::Error && s_ShowErrorMessages)
                ImGui::TextColored({0.8f, 0.4f, 0.4f, 1.0f}, "%s", message.m_MessageData.c_str());
            ImGui::Separator();
        }

        ImGui::EndChild();
        ImGui::End();
        ImGui::End();
    }

    void EditorConsole::Clear() {
        s_MessageBuffer.clear();
    }

}// namespace Aph
