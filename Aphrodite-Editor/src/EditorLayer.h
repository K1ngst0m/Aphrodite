//
// Created by npchitman on 6/24/21.
//

#ifndef Aphrodite_ENGINE_EDITORLAYER_H
#define Aphrodite_ENGINE_EDITORLAYER_H

#include <Aphrodite.hpp>

#include "Aphrodite/Renderer/EditorCamera.h"
#include "Panels/AssetBrowser.h"
#include "Panels/ContentBrowser.h"
#include "Panels/EditorConsole.h"
#include "Panels/SceneHierarchy.h"
#include "Panels/Settings.h"
#include "Panels/Status.h"

namespace Aph::Editor {

    class EditorLayer : public Layer {
    public:
        EditorLayer();
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        static std::string GetHoveredComponentName();
    private:
        void DrawSceneHierarchy();
        void DrawViewport();
        void DrawStatusData();
        void DrawMenuBar();
        void DrawToolBar();
        void DrawSettings();
        static void DrawConsole();
        static void DrawAssetBrowser();

    private:
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);

        void NewScene();
        void OpenScene();
        void SaveSceneAs();

        void OnScenePlay();
        void OnSceneStop();
        void OnScenePause();

    private:
        enum class SceneState {
            Edit = 0,
            Play = 1,
            Pause = 2
        };
        SceneState m_SceneState = SceneState::Edit;

    private:
        Ref<Framebuffer> m_Framebuffer;
        Ref<Framebuffer> m_IDFrameBuffer;

        Ref<Scene> m_ActiveScene{};
        Ref<Scene> m_EditorScene{};
        Ref<Scene> m_RuntimeScene{};

        static Entity s_HoveredEntity;

        EditorCamera m_EditorCamera;

        bool m_ViewportFocused = false, m_ViewportHovered = false, m_HasViewportEvent = false;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2]{};

        float frameTime = 0.0f;

        int m_GizmoType = -1;

        SceneHierarchy m_SceneHierarchyPanel;
        Settings m_SettingsPanel;
        Status m_StatusPanel;
    };

}// namespace Aph

#endif//Aphrodite_ENGINE_EDITORLAYER_H
