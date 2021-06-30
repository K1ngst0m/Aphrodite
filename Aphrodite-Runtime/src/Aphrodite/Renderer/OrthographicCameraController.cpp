//
// Created by npchitman on 6/21/21.
//

#include "Aphrodite/Renderer/OrthographicCameraController.h"

#include "Aphrodite/Core/Input.h"
#include "Aphrodite/Core/KeyCodes.h"
#include "pch.h"

namespace Aph {

    OrthographicCameraController::OrthographicCameraController(float aspectRation, bool rotation)
        : m_AspectRatio(aspectRation),
          m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel),
          m_Rotation(rotation) {}

    void OrthographicCameraController::OnUpdate(Timestep ts) {
        APH_PROFILE_FUNCTION();

        if (Input::IsKeyPressed(Key::A)) {
            m_CameraPosition.x -= std::cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
            m_CameraPosition.y -= std::sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        } else if (Input::IsKeyPressed(Key::D)) {
            m_CameraPosition.x += std::cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
            m_CameraPosition.y += std::sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        }

        if (Input::IsKeyPressed(Key::W)) {
            m_CameraPosition.x += -std::sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
            m_CameraPosition.y += std::cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        } else if (Input::IsKeyPressed(Key::S)) {
            m_CameraPosition.x -= -std::sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
            m_CameraPosition.y -= std::cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        }

        if (m_Rotation) {
            if (Input::IsKeyPressed(Key::Q))
                m_CameraRotation += m_CameraRotationSpeed * ts;
            if (Input::IsKeyPressed(Key::E))
                m_CameraRotation -= m_CameraRotationSpeed * ts;

            if (m_CameraRotation > 180.0f)
                m_CameraRotation -= 360.0f;
            else if (m_CameraRotation <= -180.0f)
                m_CameraRotation += 360.0f;

            m_Camera.SetRotation(m_CameraRotation);
        }

        m_Camera.SetPosition(m_CameraPosition);

        m_CameraTranslationSpeed = m_ZoomLevel;
    }

    void OrthographicCameraController::OnEvent(Event& e) {
        APH_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(APH_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(APH_BIND_EVENT_FN(OrthographicCameraController::OnWindowResized));
    }

    bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e) {
        APH_PROFILE_FUNCTION();

        m_ZoomLevel -= e.GetYOffset() * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
        m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
        return false;
    }

    bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& e) {
        APH_PROFILE_FUNCTION();

        OnResize((float) e.GetWidth(), (float) e.GetHeight());
        return false;
    }

    void OrthographicCameraController::OnResize(float width, float height) {
        m_AspectRatio = width / height;
        m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    }
}// namespace Aph
