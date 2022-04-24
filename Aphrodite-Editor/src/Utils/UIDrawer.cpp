//
// Created by npchitman on 7/8/21.
//

#include "UIDrawer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <Aphrodite.h>
#include <glm/gtc/type_ptr.hpp>

#include "pch.h"

namespace Aph::Editor {
    static int s_UIContextID = 0;
    static uint32_t s_Counter = 0;
    static char s_IDBuffer[16];

    void UIDrawer::PushID() {
        ImGui::PushID(s_UIContextID++);
        s_Counter = 0;
    }

    void UIDrawer::PopID() {
        ImGui::PopID();
        s_UIContextID--;
    }

    void UIDrawer::BeginPropertyGrid() {
        PushID();
        ImGui::Columns(2);
    }

    void UIDrawer::Property(const char* label) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        float fullWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = fullWidth * 0.6f;
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImRect textRect;
        textRect.Min = ImGui::GetCursorScreenPos();
        textRect.Max = textRect.Min;
        textRect.Max.x += fullWidth - itemWidth;
        textRect.Max.y += textSize.y;

        ImGui::SetCursorScreenPos(textRect.Min);

        ImGui::AlignTextToFramePadding();
        textRect.Min.y += window->DC.CurrLineTextBaseOffset;
        textRect.Max.y += window->DC.CurrLineTextBaseOffset;

        ImGui::ItemSize(textRect);
        if (ImGui::ItemAdd(textRect, window->GetID(label))) {
            ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x,
                                      textRect.Max.x, label, nullptr, &textSize);

            if (textRect.GetWidth() < textSize.x && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", label);
        }
        ImVec2 v(0, textSize.y + window->DC.CurrLineTextBaseOffset);
        ImGui::SetCursorScreenPos(ImVec2(textRect.Max.x - v.x, textRect.Max.y - v.y));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(itemWidth);
    }

    bool UIDrawer::Property(const char* label, std::string& value) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        char buffer[256];
        strcpy(buffer, value.c_str());

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::InputText(s_IDBuffer, buffer, 256)) {
            value = buffer;
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    void UIDrawer::Property(const char* label, const char* value) {
        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        ImGui::InputText(s_IDBuffer, (char*) value, 256, ImGuiInputTextFlags_ReadOnly);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    bool UIDrawer::Property(const char* label, int& value) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::DragInt(s_IDBuffer, &value))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool UIDrawer::Property(const char* label, int& value, int min, int max) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::SliderInt(s_IDBuffer, &value, min, max))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool UIDrawer::Property(const char* label, float& value, float delta) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::DragFloat(s_IDBuffer, &value, delta))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool UIDrawer::Property(const char* label, float& value, float min, float max, const char* fmt) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::SliderFloat(s_IDBuffer, &value, min, max, fmt))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool UIDrawer::Property(const char* label, glm::vec2& value, float delta) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::DragFloat2(s_IDBuffer, glm::value_ptr(value), delta))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool UIDrawer::Property(const char* label, bool& flag) {
        bool modified = false;

        ImGui::Text("%s", label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        s_IDBuffer[0] = '#';
        s_IDBuffer[1] = '#';
        memset(s_IDBuffer + 2, 0, 14);
        std::sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        if (ImGui::Checkbox(s_IDBuffer, &flag))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    void UIDrawer::EndPropertyGrid() {
        ImGui::Columns(1);
        PopID();
    }

    void UIDrawer::DrawGrid(const std::function<void(void)>& func) {
        Draw([&]() {
                   BeginPropertyGrid();
                   ImGui::PushStyleColor(ImGuiCol_SliderGrab, Style::Color::Foreground.at("Second")); },
             [&]() {
                 ImGui::PopStyleColor();
                 EndPropertyGrid();
             },
             [&]() {
                 func();
             });
    }

    void UIDrawer::DrawGrid(const std::string& name, const APH_UIFUNC& func) {
        Draw([&]() {
                   BeginPropertyGrid();
                   ImGui::PushStyleColor(ImGuiCol_SliderGrab, Style::Color::Foreground.at("Primary"));
                   ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Style::Color::Foreground.at("Second"));
               ImGui::PushID(name.c_str());
             },
             [&]() {
                 ImGui::PopID();
                 ImGui::PopStyleColor(2);
                 EndPropertyGrid();
             },
             [&]() {
                 func();
             });
    }

    void UIDrawer::Draw(const APH_UIFUNC& uiPushFunc,
                        const APH_UIFUNC& uiPopFunc,
                        const APH_UIFUNC& uiDrawFunc) {
        uiPushFunc();
        uiDrawFunc();
        uiPopFunc();
    }
}// namespace Aph::Editor
