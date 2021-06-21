//
// Created by npchitman on 6/21/21.
//

#ifndef HAZEL_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H
#define HAZEL_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H

#include "Hazel/Core/TimeStep.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/OrthographicCamera.h"

namespace Hazel {
    class OrthographicCameraController {
    public:
        explicit OrthographicCameraController(float aspectRation, bool rotation = false);
        void OnUpdate(Timestep ts);
        void OnEvent(Event& e);

        OrthographicCamera& GetCamera() { return m_Camera; }
        const OrthographicCamera& GetCamera() const { return m_Camera; }

        float GetZoomLevel() const { return m_ZoomLevel; }
        void SetZoomLevel(float level) { m_ZoomLevel = level; }

    private:
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

    private:
        float m_AspectRatio;
        float m_ZoomLevel = 1.0f;
        OrthographicCamera m_Camera;

        bool m_Rotation;

        glm::vec3 m_CameraPosition{0.0f, 0.0f, 0.0f};
        float m_CameraRotation = 0.0f;
        float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 180.0f;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H
