//
// Created by npchitman on 6/21/21.
//

#ifndef Aphrodite_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H
#define Aphrodite_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H

#include "Aphrodite/Core/TimeStep.h"
#include "Aphrodite/Events/ApplicationEvent.h"
#include "Aphrodite/Events/MouseEvent.h"
#include "Aphrodite/Renderer/OrthographicCamera.h"

namespace Aph {
    class OrthographicCameraController {
    public:
        explicit OrthographicCameraController(float aspectRation, bool rotation = false);
        void OnUpdate(Timestep ts);
        void OnEvent(Event& e);

        void OnResize(float width, float height);

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
}// namespace Aph


#endif//Aphrodite_ENGINE_ORTHOGRAPHICCAMERACONTROLLER_H
