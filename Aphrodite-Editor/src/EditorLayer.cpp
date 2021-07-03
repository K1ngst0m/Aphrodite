//
// Created by npchitman on 6/24/21.
//

#include "EditorLayer.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Aphrodite/Math/Math.h"
#include "Aphrodite/Scene/SceneSerializer.h"
#include "Aphrodite/Utils/PlatformUtils.h"

namespace Aph {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer"),
          m_CameraController(1280.0f / 720.0f) {
    }

    void EditorLayer::OnAttach() {
        APH_PROFILE_FUNCTION();

        ImGuiConsole::Log("This is a log statement");
        ImGuiConsole::LogWarning("This is a warning statement");
        ImGuiConsole::LogError("This is an error statement");
        ImGuiConsole::Log("This is a log statement with parameter: %s, %d, %f", "abc", 34, 6.0f);

        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth};
        fbSpec.Width = 1280;
        fbSpec.Height = 720;

        m_Framebuffer = Framebuffer::Create(fbSpec);

        m_ActiveScene = CreateRef<Scene>();

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if(commandLineArgs.Count > 1){
            auto sceneFilePath = commandLineArgs[1];
            SceneSerializer serializer(m_ActiveScene);
            serializer.Deserialize(sceneFilePath);
        }

        m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        ImGuiAssetBrowser::Init();
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
            m_Framebuffer->Resize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
            m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
            m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
        }

        if (m_ViewportFocused)
            m_CameraController.OnUpdate(ts);

        m_EditorCamera.OnUpdate(ts);

        // Render
        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::SetClearColor(Aph::Style::Color::ClearColor);
        RenderCommand::Clear();
        m_Framebuffer->Bind();

        m_Framebuffer->ClearAttachment(1, -1);

        // Update scene
        m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);

        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

        my = viewportSize.y - my;

        int mouseX = static_cast<int>(mx);
        int mouseY = static_cast<int>(my);

        if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(viewportSize.x) && mouseY < static_cast<int>(viewportSize.y)) {
            int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
            m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity) pixelData, m_ActiveScene.get());
        }

        m_Framebuffer->UnBind();
    }

    void EditorLayer::OnImGuiRender() {
        APH_PROFILE_FUNCTION();

        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
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

            ImGui::EndMenuBar();
        }


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

        // Viewport
        ImGui::Begin(Style::Title::Viewport.data());
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
        m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};

        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID(0);
        ImGui::Image(reinterpret_cast<void*>(textureID),
                     ImVec2{m_ViewportSize.x, m_ViewportSize.y},
                     ImVec2{0, 1}, ImVec2{1, 0});

        // Gizmos
        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                              m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                              m_ViewportBounds[1].y - m_ViewportBounds[0].y);


            // Camera
            // Runtime camera from entity
//            auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
//            const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
//            const glm::mat4& cameraProjection = camera.GetProjection();
//            glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

            // Editor camera
            const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            // Entity transform
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            // Snapping
            bool snap = Input::IsKeyPressed(Key::LeftControl);
            float snapValue = 0.5f;// Snap to 0.5m for translation/scale
            // Snap to 45 degrees for rotation
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[3] = {snapValue, snapValue, snapValue};

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                 (ImGuizmo::OPERATION) m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                                 nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing()) {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();

        // Scene Hierarchy && Content Browser
        m_SceneHierarchyPanel.OnImGuiRender();
//        m_ContentBrowserPanel.OnImGuiRender();

        ImGui::Begin(Style::Title::RenderInfo.data());
        ImGui::Text("# Vendor         : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Vendor);
        ImGui::Text("# Hardware       : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Renderer);
        ImGui::Text("# OpenGL Version : %s", Application::Get().GetWindow().GetGraphicsContextInfo().Version);
        ImGui::End();

        // Mouse Hover
        std::string name = "None";
        if (m_HoveredEntity) name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
        ImGui::Text("# Hovered Entity: %s", name.c_str());

        // Statistics
        ImGui::Begin(Style::Title::Renderer2DStatistics.data());
        auto stats = Renderer2D::GetStats();
        ImGui::Text("# Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("# Quads: %d", stats.QuadCount);
        ImGui::Text("# Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("# Indices: %d", stats.GetTotalIndexCount());
        ImGui::End();


        // Console
        ImGui::Begin(Style::Title::Console.data());
        ImGuiConsole::Draw();
        ImGui::End();

        // Asset Browser
        ImGui::Begin(Style::Title::Project.data());
        ImGuiAssetBrowser::Draw();
        ImGui::End();

        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& e) {
        m_CameraController.OnEvent(e);
        m_EditorCamera.OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(APH_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(APH_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e) {
        // Shortcuts
        if (e.GetRepeatCount() > 0)
            return false;

        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

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

    void EditorLayer::NewScene() {
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize((uint32_t) m_ViewportSize.x, (uint32_t) m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OpenScene() {
        auto filepath = FileDialogs::OpenFile("Aphrodite Scene (*.sce)\0*.sce\0");
        if (!filepath.empty()) {
            m_ActiveScene = CreateRef<Scene>();
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

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        if (e.GetMouseButton() == Mouse::ButtonLeft) {
            if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
                m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
        }
        return false;
    }
}// namespace Aph