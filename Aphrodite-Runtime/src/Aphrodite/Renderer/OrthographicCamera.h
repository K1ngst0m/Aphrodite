//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_ORTHOGRAPHICCAMERA_H
#define Aphrodite_ENGINE_ORTHOGRAPHICCAMERA_H

#include <glm/glm.hpp>

namespace Aph {
    class OrthographicCamera {
    public:
        OrthographicCamera(float left, float right, float bottom, float top);

        const glm::vec3 &GetPosition() const { return m_Position; }

        void SetPosition(const glm::vec3 &position) {
            m_Position = position;
            RecalculateViewMatrix();
        }

        float GetRotation() const { return m_Rotation; }

        void SetProjection(float left, float right, float bottom, float top);

        void SetRotation(float rotation) {
            m_Rotation = rotation;
            RecalculateViewMatrix();
        }

        const glm::mat4 &GetProjectionMatrix() const { return m_ProjectionMatrix; }

        const glm::mat4 &GetViewMatrix() const { return m_ViewMatrix; }

        const glm::mat4 &GetViewProjectionMatrix() const {
            return m_ViewProjectionMatrix;
        }

    private:
        void RecalculateViewMatrix();

    private:
        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ViewProjectionMatrix;

        glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
        float m_Rotation = 0.0f;
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_ORTHOGRAPHICCAMERA_H
