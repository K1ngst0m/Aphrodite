#include "camera.h"

namespace aph
{

void Camera::updateView()
{
    // rotation
    glm::mat4 rotM = glm::mat4(1.0f);
    {
        rotM = glm::rotate(rotM, glm::radians(m_rotation.x * (m_flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    // translation
    glm::mat4 transM = glm::mat4(1.0f);
    {
        glm::vec3 translation = -m_position;
        if(m_flipY) { translation.y *= -1.0f; }
        transM = glm::translate(transM, translation);
    }

    if(m_cameraType == CameraType::PERSPECTIVE) { m_view = rotM * transM; }
    else if(m_cameraType == CameraType::ORTHO) { m_view = transM * rotM; }
};

void Camera::updateMovement(float deltaTime)
{
    if(m_cameraType == CameraType::PERSPECTIVE)
    {
        if(std::any_of(m_keys.begin(), m_keys.end(), [](const auto& key) -> bool { return key.second; }))
        {
            glm::vec3 camFront{-cos(glm::radians(m_rotation.x)) * sin(glm::radians(m_rotation.y)),
                               sin(glm::radians(m_rotation.x)),
                               cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y))};
            camFront = glm::normalize(camFront);

            float moveSpeed{deltaTime * m_movementSpeed};

            if(m_keys[Direction::UP]) m_position += camFront * moveSpeed;
            if(m_keys[Direction::DOWN]) m_position -= camFront * moveSpeed;
            if(m_keys[Direction::LEFT])
                m_position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            if(m_keys[Direction::RIGHT])
                m_position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
        }
    }
};

void PerspectiveCamera::updateProj()
{
    m_projection        = glm::perspective(glm::radians(m_perspective.fov), m_aspect, m_perspective.znear, m_perspective.zfar);
    if(m_flipY) { m_projection[1][1] *= -1.0f; }
};

void OrthoCamera::updateProj()
{
    m_projection   = glm::ortho(m_ortho.left, m_ortho.right, m_ortho.bottom, m_ortho.top, m_ortho.front, m_ortho.back);
}
}  // namespace aph
