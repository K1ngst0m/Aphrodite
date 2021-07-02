//
// Created by npchitman on 5/31/21.
//

#include "Aphrodite/ImGui/ImGuiLayer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include "Aphrodite/Core/Application.h"
#include "Aphrodite/Fonts/IconsFontAwesome5Pro.h"
#include "GLFW/glfw3.h"
#include "pch.h"

namespace Aph {
    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}
    ImGuiLayer::~ImGuiLayer() = default;

    void ImGuiLayer::OnAttach() {
        APH_PROFILE_FUNCTION();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
//        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;// Enable Multi-Viewport /
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        float fontSize = 24.0f;

        io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);

        static const ImWchar icons_ranges_fontawesome[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config_fontawesome;
        icons_config_fontawesome.MergeMode = true;
        icons_config_fontawesome.PixelSnapH = true;

        io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, fontSize, &icons_config_fontawesome, icons_ranges_fontawesome);

        io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);
        io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, fontSize, &icons_config_fontawesome, icons_ranges_fontawesome);

        // Setup Dear ImGui style
#if 1
        ImGui::StyleColorsDark();
#else
        //ImGui::StyleColorsClassic();
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
        ImGui_ImplOpenGL3_Init("#version 460");
    }

    void ImGuiLayer::OnDetach() {
        APH_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event &e) {
        if (m_BlockEvents) {
            ImGuiIO &io = ImGui::GetIO();
            e.Handled |= e.IsInCateGory(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCateGory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin() {
        APH_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::End() {
        APH_PROFILE_FUNCTION();

        auto &io = ImGui::GetIO();
        auto &app = Application::Get();
        io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow().GetWidth()),
                                static_cast<float>(app.GetWindow().GetHeight()));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            auto* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void ImGuiLayer::SetDarkThemeColors() {

        // color style
        const auto foreground_1 = ImVec4{0.8f, 0.6f, 0.53f, 1.0f};
        const auto foreground_2 = ImVec4{0.406f, 0.738f, 0.687f, 1.0f};
        const auto background_1 = ImVec4{0.079f, 0.115f, 0.134f, 1.0f};
        const auto background_2 = ImVec4{0.406f, 0.738f, 0.687f, 1.0f};
        const auto background_hovered = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        const auto background_active = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        auto &colors = ImGui::GetStyle().Colors;

        // Text
        colors[ImGuiCol_Text] = foreground_1;

        // Window
        colors[ImGuiCol_WindowBg] = background_1;

        // MenuBar
        colors[ImGuiCol_MenuBarBg] = background_1;

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = background_hovered;
        colors[ImGuiCol_HeaderActive] = background_active;

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = background_hovered;
        colors[ImGuiCol_ButtonActive] = background_active;

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = background_hovered;
        colors[ImGuiCol_FrameBgActive] = background_active;

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = background_1;
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    }
}// namespace Aph
