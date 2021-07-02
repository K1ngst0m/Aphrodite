//
// Created by npchitman on 6/24/21.
//

#ifndef Aphrodite_ENGINE_EDITORLAYER_H
#define Aphrodite_ENGINE_EDITORLAYER_H

#include "Aphrodite.h"
#include "Aphrodite/Renderer/EditorCamera.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

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

    private:
        Aph::OrthographicCameraController m_CameraController;

        // Temp
        Ref<VertexArray> m_SquareVA;
        Ref<Shader> m_FlatColorShader;
        Ref<Framebuffer> m_Framebuffer;

        Ref<Scene> m_ActiveScene{};

        Entity m_SquareEntity{};
        Entity m_CameraEntity{};
        Entity m_HoveredEntity{};

        bool m_ViewportFocused = false, m_ViewportHovered = false;

        bool m_PrimaryCamera = true;

        EditorCamera m_EditorCamera;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2]{};

        int m_GizmoType = -1;

        SceneHierarchyPanel m_SceneHierarchyPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
    };

}// namespace Aph

#endif//Aphrodite_ENGINE_EDITORLAYER_H
