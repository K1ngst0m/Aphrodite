//
// Created by npchitman on 5/31/21.
//

#include "Aphrodite/UI/UILayer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include "Aphrodite/Core/Application.h"
#include "Aphrodite/Fonts/IconsFontAwesome5Pro.h"
#include "GLFW/glfw3.h"
#include "pch.h"

namespace Aph {
    UILayer::UILayer() : Layer("UILayer") {}
    UILayer::~UILayer() = default;

    void UILayer::OnAttach() {
        APH_PROFILE_FUNCTION();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
        // TODO: Enable Gamepad
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;// Enable Multi-Viewport /

        float fontSize = 22.0f;
        float iconSize = 17.0f;

        static const ImWchar icons_ranges_fontawesome[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config_fontawesome;
        icons_config_fontawesome.MergeMode = true;
        icons_config_fontawesome.PixelSnapH = true;

        io.Fonts->AddFontFromFileTTF(FONT_UI, fontSize);
        io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconSize, &icons_config_fontawesome, icons_ranges_fontawesome);
        io.FontDefault = io.Fonts->AddFontFromFileTTF(FONT_UI, fontSize);
        io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconSize, &icons_config_fontawesome, icons_ranges_fontawesome);

#if 1
        ImGui::StyleColorsDark();
#else
        //UI::StyleColorsClassic();
#endif

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        SetDarkThemeColors();

        Application &app = Application::Get();
        auto *window = static_cast<GLFWwindow *>(app.GetWindow().GetNativeWindow());

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void UILayer::OnDetach() {
        APH_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void UILayer::OnEvent(Event &e) {
        if (m_BlockEvents) {
            ImGuiIO &io = ImGui::GetIO();
            e.Handled |= e.IsInCateGory(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCateGory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void UILayer::Begin() {
        APH_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void UILayer::End() {
        APH_PROFILE_FUNCTION();

        auto &io = ImGui::GetIO();
        auto &app = Application::Get();
        io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow().GetWidth()),
                                static_cast<float>(app.GetWindow().GetHeight()));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            auto *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void UILayer::SetDarkThemeColors() {

        auto &colors = ImGui::GetStyle().Colors;

        // Text
        colors[ImGuiCol_Text] = Style::Color::foreground_1;

        // Window
        colors[ImGuiCol_WindowBg] = Style::Color::background_1;

        // MenuBar
        colors[ImGuiCol_MenuBarBg] = Style::Color::background_1;

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = Style::Color::background_hovered;
        colors[ImGuiCol_HeaderActive] = Style::Color::background_active;

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = Style::Color::background_hovered;
        colors[ImGuiCol_ButtonActive] = Style::Color::background_active;

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = Style::Color::background_hovered;
        colors[ImGuiCol_FrameBgActive] = Style::Color::background_active;

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = Style::Color::background_1;
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    }
}// namespace Aph
