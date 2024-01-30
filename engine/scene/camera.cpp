#include "camera.h"

namespace aph
{
Camera::Camera(CameraType cameraType) :
    Object{Id::generateNewId<Camera>(), ObjectType::CAMERA},
    m_cameraType(cameraType)
{
}

void CameraController::updateView()
{
    // rotation
    glm::mat4 rotM = glm::mat4(1.0f);
    {
        rotM = glm::rotate(rotM, glm::radians(m_direction.x * (m_flipY ? -1.0f : 1.0f)), {1.0f, 0.0f, 0.0f});
        rotM = glm::rotate(rotM, glm::radians(m_direction.y), {0.0f, 1.0f, 0.0f});
        rotM = glm::rotate(rotM, glm::radians(m_direction.z), {0.0f, 0.0f, 1.0f});
    }

    // translation
    glm::mat4 transM = glm::mat4(1.0f);
    {
        glm::vec3 translation = -m_position;
        if(m_flipY)
        {
            translation.y *= -1.0f;
        }
        transM = glm::translate(transM, translation);
    }

    if(m_camera->m_cameraType == CameraType::PERSPECTIVE)
    {
        m_camera->m_view = rotM * transM;
    }
    else if(m_camera->m_cameraType == CameraType::ORTHO)
    {
        m_camera->m_view = transM * rotM;
    }
};

void CameraController::update(float deltaTime)
{
    updateView();
    updateProj();
    if(m_camera->m_cameraType == CameraType::PERSPECTIVE)
    {
        if(std::any_of(m_directions.begin(), m_directions.end(), [](const auto& key) -> bool { return key.second; }))
        {
            glm::vec3 camFront{-cos(glm::radians(m_direction.x)) * sin(glm::radians(m_direction.y)),
                               sin(glm::radians(m_direction.x)),
                               cos(glm::radians(m_direction.x)) * cos(glm::radians(m_direction.y))};
            camFront = glm::normalize(camFront);

            float moveSpeed{deltaTime * m_movementSpeed};

            if(m_directions[Direction::UP])
                m_position += camFront * moveSpeed;
            if(m_directions[Direction::DOWN])
                m_position -= camFront * moveSpeed;
            if(m_directions[Direction::LEFT])
                m_position -= glm::normalize(glm::cross(camFront, {0.0f, 1.0f, 0.0f})) * moveSpeed;
            if(m_directions[Direction::RIGHT])
                m_position += glm::normalize(glm::cross(camFront, {0.0f, 1.0f, 0.0f})) * moveSpeed;
        }
    }
};

void CameraController::updateProj()
{
    if(m_camera->m_cameraType == CameraType::PERSPECTIVE)
    {
        m_camera->m_projection = glm::perspective(glm::radians(m_camera->m_perspective.fov), m_camera->m_aspect,
                                                  m_camera->m_perspective.znear, m_camera->m_perspective.zfar);
        if(m_flipY)
        {
            m_camera->m_projection[1][1] *= -1.0f;
        }
    }
    else if(m_camera->m_cameraType == CameraType::ORTHO)
    {
        m_camera->m_projection = glm::ortho(m_camera->m_ortho.left, m_camera->m_ortho.right, m_camera->m_ortho.bottom,
                                            m_camera->m_ortho.top, m_camera->m_ortho.front, m_camera->m_ortho.back);
    }
}
}  // namespace aph
