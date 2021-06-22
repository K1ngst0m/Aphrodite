//
// Created by npchitman on 5/31/21.
//

#include "Hazel/ImGui/ImGuiLayer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "GLFW/glfw3.h"
#include "Hazel/Core/Application.h"
#include "hzpch.h"

Hazel::ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

Hazel::ImGuiLayer::~ImGuiLayer() = default;

void Hazel::ImGuiLayer::OnAttach() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
    // Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;// Enable Multi-Viewport /
                                                       // Platform Windows
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    Application &app = Application::Get();
    auto *window = static_cast<GLFWwindow *>(app.GetWindow().GetNativeWindow());

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void Hazel::ImGuiLayer::OnDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Hazel::ImGuiLayer::OnImGuiRender() { Layer::OnImGuiRender(); }

void Hazel::ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Hazel::ImGuiLayer::End() {
    auto &io = ImGui::GetIO();
    auto &app = Application::Get();
    io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow().GetWidth()),
                            static_cast<float>(app.GetWindow().GetHeight()));

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        auto backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}
