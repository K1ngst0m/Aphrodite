//
// Created by npchitman on 6/24/21.
//

#include "EditorLayer.h"

#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <Aphrodite.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Aph::Editor {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer") {}

    void EditorLayer::OnAttach() {
        APH_PROFILE_FUNCTION();
        EditorConsole::Log("Aphrodite Engine is Running");

        //#ifdef APH_DEBUG
        // Log Example
        EditorConsole::Log("A log example");
        EditorConsole::LogWarning("A warning example");
        EditorConsole::LogError("An error example");
        EditorConsole::Log("A log example with parameter: {}, {}, {}", "abc", 34, 6.0f);
        //#endif

        // frame buffer
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {FramebufferTextureFormat::RGBA8,
                              FramebufferTextureFormat::RED_INTEGER,
                              FramebufferTextureFormat::Depth};
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);

        // scene
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;

        m_EditorCamera = EditorCamera(60.0f,
                                      fbSpec.Width / fbSpec.Height,
                                      0.1f, 1000.0f);

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        // Asset Browser Init
//        AssetBrowser::Init();

        // command line args
        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 1) {
            auto sceneFilePath = commandLineArgs[1];
            SceneSerializer serializer(m_ActiveScene);
            serializer.Deserialize(sceneFilePath);
        }
    }

    void EditorLayer::OnDetach() {
        APH_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Timestep ts) {
        APH_PROFILE_FUNCTION();

        // Resize
        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
            (static_cast<float>(spec.Width) != m_ViewportSize.x || static_cast<float>(spec.Height) != m_ViewportSize.y)) {

            m_Framebuffer->Resize(static_cast<uint32_t>(m_ViewportSize.x),
                                  static_cast<uint32_t>(m_ViewportSize.y));

            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
            m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x),
                                            static_cast<uint32_t>(m_ViewportSize.y));
        }

        // Render
        m_Framebuffer->Bind();
        Renderer2D::ResetStats();
        RenderCommand::SetClearColor(Aph::Style::Color::Clear);
        RenderCommand::Clear();

        // Update scene
        m_EditorCamera.OnUpdate(ts);
        switch (m_SceneState) {
            case SceneState::Play: {
                m_ActiveScene->OnRuntimeUpdate(ts);
                m_ActiveScene->OnEditorUpdate(ts, m_EditorCamera);
                break;
            }
            case SceneState::Pause: {
                // TODO
                m_ActiveScene->OnRuntimePause(ts);
                break;
            }
            case SceneState::Edit: {
                m_ActiveScene->OnEditorUpdate(ts, m_EditorCamera);
                break;
            }
        }

        MousePicking();

        m_Framebuffer->UnBind();
    }

    // Draw UI
    void EditorLayer::OnUIRender() {
        APH_PROFILE_FUNCTION();
        DrawEditor([&]() {
            DrawMenuBar();
            DrawToolBar();
            DrawScene();
            DrawSceneHierarchy();
            DrawStatusData();
            DrawConsole();
            DrawAssetBrowser();
            DrawSettings();
        });
    }

    void EditorLayer::OnEvent(Event& e) {
        m_EditorCamera.OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(APH_BIND_EVENT_FN(OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(APH_BIND_EVENT_FN(OnMouseButtonPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(APH_BIND_EVENT_FN(OnMouseButtonReleased));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e) {
        // Shortcuts
        if (e.GetRepeatCount() > 0)
            return false;

        bool control = Input::IsKeyPressed(Key::LeftControl) ||
                       Input::IsKeyPressed(Key::RightControl);

        bool shift = Input::IsKeyPressed(Key::LeftShift) ||
                     Input::IsKeyPressed(Key::RightShift);

        switch (e.GetKeyCode()) {
            case Key::N: {
                if (control)
                    NewScene();

                break;
            }
            case Key::O: {
                if (control)
                    OpenScene();

                break;
            }
            case Key::S: {
                if (control && shift)
                    SaveSceneAs();

                break;
            }

            // Gizmos
            case Key::Q: {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = -1;
                break;
            }
            case Key::W: {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E: {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R: {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
        }
        return true;
    }

    void EditorLayer::OnScenePlay() {
        EditorConsole::Log("Scene Play");
        m_SceneState = SceneState::Play;

        m_RuntimeScene = CreateRef<Scene>();
        m_EditorScene->CopyTo(m_RuntimeScene);
        m_ActiveScene = m_RuntimeScene;

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        m_ActiveScene->OnRuntimeStart();
    }

    void EditorLayer::OnSceneStop() {
        EditorConsole::Log("Scene Stop");
        m_SceneState = SceneState::Edit;
        m_ActiveScene = m_EditorScene;

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        m_ActiveScene->OnRuntimeEnd();
        m_RuntimeScene = nullptr;
    }

    void EditorLayer::OnScenePause() {
        // TODO: on scene pause
        m_SceneState = SceneState::Pause;
        EditorConsole::Log("Scene Pause");
    }

    void EditorLayer::NewScene() {
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_ActiveScene->OnViewportResize((uint32_t) m_ViewportSize.x, (uint32_t) m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OpenScene() {
        auto filepath = FileDialogs::OpenFile("Aphrodite Scene (*.sce) *.sce ");
        if (!filepath.empty()) {
            m_EditorScene = CreateRef<Scene>();
            m_ActiveScene = m_EditorScene;
            m_ActiveScene->OnViewportResize((uint32_t) m_ViewportSize.x, (uint32_t) m_ViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_ActiveScene);

            SceneSerializer serializer(m_ActiveScene);
            serializer.Deserialize(filepath);
        }
    }

    void EditorLayer::SaveSceneAs() {
        auto filepath = FileDialogs::SaveFile("Aph Scene (*.sce)\0*.sce\0");
        if (!filepath.empty()) {
            SceneSerializer serializer(m_ActiveScene);
            serializer.Serialize(filepath);
        }
    }

    bool EditorLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
        if (e.GetMouseButton() == Mouse::ButtonRight) {
            Application::Get().GetWindow().EnableCursor();
            m_HasViewportEvent = false;
        }

        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        if (e.GetMouseButton() == Mouse::ButtonLeft) {
            if (m_ViewportHovered &&
                !ImGuizmo::IsOver() &&
                !Input::IsKeyPressed(Key::LeftAlt))
                m_SceneHierarchyPanel.SetSelectedEntity(s_HoveredEntity);
        }
        if (e.GetMouseButton() == Mouse::ButtonRight) {
            m_HasViewportEvent = true;
            Application::Get().GetWindow().DisableCursor();
        }
        return false;
    }

    ////////////////
    // UI Element //
    ////////////////

    void EditorLayer::DrawSceneHierarchy() {
        m_SceneHierarchyPanel.OnUIRender();
    }

    void EditorLayer::DrawScene() {
        // Viewport
        ImGui::Begin(Style::Title::Viewport.data());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

        if (m_SceneState == SceneState::Play ||
            m_SceneState == SceneState::Pause) {
            auto color = (const glm::vec4&) Style::Color::Foreground.at("Second");
            ImVec2 windowMin = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowMax = {windowMin.x + windowSize.x,
                                windowMin.y + windowSize.y};
            ImGui::GetForegroundDrawList()->AddRect(windowMin, windowMax,
                                                    ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w)));
        }

        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();

        m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x,
                               viewportMinRegion.y + viewportOffset.y};
        m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x,
                               viewportMaxRegion.y + viewportOffset.y};

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused && !m_ViewportHovered && !m_HasViewportEvent);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};

        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(reinterpret_cast<void*>(textureID),
                     ImVec2{m_ViewportSize.x, m_ViewportSize.y},
                     ImVec2{0, 1}, ImVec2{1, 0});

        // Gizmos
        auto selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1 &&
            !Input::IsKeyPressed(Key::LeftAlt) &&
            m_SceneState == SceneState::Edit) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                              m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                              m_ViewportBounds[1].y - m_ViewportBounds[0].y);

            // Editor camera
            const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            // Entity transform
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            // Snapping
            bool snap = Input::IsKeyPressed(Key::LeftControl);
            float snapValue = 0.5f;// Snap to 0.5m for translation/scale
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[3] = {snapValue, snapValue, snapValue};

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                 (ImGuizmo::OPERATION) m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                                 nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing()) {
                auto [translation, rotation, scale] =
                        Math::DecomposeTransform(transform);

                glm::vec3 deltaRotation = rotation - tc.Rotation;

                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }

    void EditorLayer::DrawStatusData() {
        m_StatusPanel.OnUIRender();
    }

    void EditorLayer::DrawConsole() {
        EditorConsole::Draw();
    }

    void EditorLayer::DrawAssetBrowser() {
        AssetBrowser::Draw();
    }

    void EditorLayer::DrawSettings() {
        m_SettingsPanel.OnUIRender();
    }

    void EditorLayer::DrawEditor(const std::function<void()>& drawlist) {
        static bool dockspaceOpen = false;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | // NOLINT(bugprone-suspicious-enum-usage)
                                                    ImGuiDockNodeFlags_NoWindowMenuButton |
                                                    ImGuiDockNodeFlags_NoCloseButton;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar |
                                        ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoBringToFrontOnFocus |
                            ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockSpace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockSpace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        style.WindowMinSize.x = minWinSizeX;

        // draw
        drawlist();

        // end
        ImGui::End();
    }

    void EditorLayer::DrawMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewScene();

                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    OpenScene();

                if (ImGui::MenuItem("Save As...", "Ctrl+Alt+S"))
                    SaveSceneAs();

                if (ImGui::MenuItem("Exit"))
                    Application::Get().Close();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    // TODO
                }

                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    // TODO
                }

                if (ImGui::MenuItem("Paste", "Ctrl+P")) {
                    // TODO
                }

                ImGui::Separator();

                if (ImGui::MenuItem("TODO 1")) {
                    // TODO
                }
                if (ImGui::MenuItem("TODO 2")) {
                    // TODO
                }
                if (ImGui::MenuItem("TODO 3")) {
                    // TODO
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Project Setting"))
                    NewScene();

                if (ImGui::MenuItem("Editor Setting"))
                    OpenScene();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EditorLayer::DrawToolBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, Style::Color::Foreground.at("Second"));
        ImGui::Begin("Toolbar", nullptr);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2);
        const ImVec2 toolbarButtonSize = {28, 28};

        ImGui::Columns(3, "Toolbar", false);
        // Transform
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::Button("\uf05b", toolbarButtonSize)) {
            if (!ImGuizmo::IsUsing())
                m_GizmoType = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("\uF0B2", toolbarButtonSize)) {
            if (!ImGuizmo::IsUsing())
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::Button("\uF021", toolbarButtonSize)) {
            if (!ImGuizmo::IsUsing())
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::Button("\uF065", toolbarButtonSize)) {
            if (!ImGuizmo::IsUsing())
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
        }
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 2.1);
        ImGui::NextColumn();
        ImGui::PopStyleColor();

        // Play, Pause, Stop
        if (m_SceneState == SceneState::Edit) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (ImGui::Button("\uf04b", toolbarButtonSize)) {
                OnScenePlay();
            }
            ImGui::SameLine();
            if (ImGui::Button("\uf04c", toolbarButtonSize)) {
            }
            ImGui::SameLine();
            if (ImGui::Button("\uf04d", toolbarButtonSize)) {
            }
            ImGui::PopStyleColor();
        } else if (m_SceneState == SceneState::Play) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::Button("\uf04b", toolbarButtonSize)) {
                OnSceneStop();
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            if (ImGui::Button("\uf04c", toolbarButtonSize)) {
                OnScenePause();
            }
            ImGui::SameLine();
            if (ImGui::Button("\uf04d", toolbarButtonSize)) {
                OnSceneStop();
            }
        } else {
            // TODO: scene pause
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::Button("\uf04b", toolbarButtonSize)) {
                OnSceneStop();
            }
            ImGui::SameLine();
            if (ImGui::Button("\uf04c", toolbarButtonSize)) {
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            if (ImGui::Button("\uf04d", toolbarButtonSize)) {
                OnSceneStop();
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    Entity EditorLayer::s_HoveredEntity = {};
    std::string EditorLayer::GetHoveredComponentName() {
        return s_HoveredEntity ? s_HoveredEntity.GetComponent<TagComponent>().Tag
                               : "None";
    }

    void EditorLayer::MousePicking() {
        // Mouse Picking
        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

        my = viewportSize.y - my;

        const int mouseX = static_cast<int>(mx);
        const int mouseY = static_cast<int>(my);

        if (mouseX >= 0 &&
            mouseY >= 0 &&
            mouseX < static_cast<int>(viewportSize.x) &&
            mouseY < static_cast<int>(viewportSize.y))
        {
            int entityId = m_Framebuffer->ReadPixel(1, mouseX, mouseY);

            if (entityId == -1 || !m_ActiveScene->HasEntity(entityId)) {
                s_HoveredEntity = Entity();
            } else {
                s_HoveredEntity = Entity(static_cast<entt::entity>(entityId), m_ActiveScene.get());
            }
        }
    }

}// namespace Aph::Editor
