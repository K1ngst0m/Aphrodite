//
// Created by npchitman on 6/24/21.
//

#ifndef Aphrodite_ENGINE_EDITORLAYER_H
#define Aphrodite_ENGINE_EDITORLAYER_H

#include <Aphrodite.hpp>
#include "Aphrodite/Renderer/EditorCamera.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/SceneHierarchyPanel.h"

namespace Aph {

    class EditorLayer : public Layer {
    public:
        EditorLayer();
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        void NewScene();
        void OpenScene();
        void SaveSceneAs();

        void OnScenePlay();
        void OnSceneStop();
        void OnScenePause();

    private:
        enum class SceneState{
            Edit = 0, Play = 1, Pause = 2
        };
        SceneState m_SceneState = SceneState::Edit;

    private:
        Aph::OrthographicCameraController m_CameraController;

        Ref<Framebuffer> m_Framebuffer;
        Ref<Framebuffer> m_IDFrameBuffer;

        Ref<Scene> m_ActiveScene{};
        Ref<Scene> m_EditorScene{};
        Ref<Scene> m_RuntimeScene{};

        Entity m_HoveredEntity{};

        EditorCamera m_EditorCamera;

        bool m_ViewportFocused = false, m_ViewportHovered = false;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2]{};

        float frameTime = 0.0f;

        int m_GizmoType = -1;

        SceneHierarchyPanel m_SceneHierarchyPanel;
    };

}// namespace Aph

#endif//Aphrodite_ENGINE_EDITORLAYER_H
