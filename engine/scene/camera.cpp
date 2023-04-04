#include "camera.h"

#include <glm/gtx/string_cast.hpp>

namespace aph
{

struct CameraDataLayout
{
    glm::mat4 view{ 1.0f };
    glm::mat4 proj{ 1.0f };
    glm::vec4 position{ 1.0f };
};

void Camera::load()
{
    updateViewMatrix();
    dataSize = sizeof(CameraDataLayout);
    data = std::make_shared<CameraDataLayout>();
    auto pData{ std::static_pointer_cast<CameraDataLayout>(data) };
    pData->view = m_matrices.view;
    pData->proj = m_matrices.perspective;
    pData->position = glm::vec4(m_position, 1.0f);
}

void Camera::update(float deltaTime)
{
    processMovement(deltaTime);
    updateViewMatrix();

    updated = true;
    auto pData{ std::static_pointer_cast<CameraDataLayout>(data) };
    pData->view = m_matrices.view;
    pData->proj = m_matrices.perspective;
    pData->position = glm::vec4(m_position, 1.0f);
}

void Camera::setAspectRatio(float aspect)
{
    m_matrices.perspective = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
    if(m_flipY)
    {
        m_matrices.perspective[1][1] *= -1.0f;
    }
}

void Camera::setPerspective(float fov, float aspect, float znear, float zfar)
{
    m_fov = fov;
    m_znear = znear;
    m_zfar = zfar;
    m_matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    if(m_flipY)
    {
        m_matrices.perspective[1][1] *= -1.0f;
    }
};

void Camera::updateViewMatrix()
{
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, glm::radians(m_rotation.x * (m_flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = -m_position;
    if(m_flipY)
    {
        translation.y *= -1.0f;
    }
    transM = glm::translate(glm::mat4(1.0f), translation);

    if(m_cameraType == CameraType::FIRSTPERSON)
    {
        m_matrices.view = rotM * transM;
    }
    else
    {
        m_matrices.view = transM * rotM;
    }

    updated = true;
};

bool Camera::isMoving() const
{
    return std::any_of(m_keys.begin(), m_keys.end(), [](const auto &key) -> bool { return key.second; });
}

void Camera::processMovement(float deltaTime)
{
    updated = false;
    if(m_cameraType == CameraType::FIRSTPERSON)
    {
        if(isMoving())
        {
            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(m_rotation.x)) * sin(glm::radians(m_rotation.y));
            camFront.y = sin(glm::radians(m_rotation.x));
            camFront.z = cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));
            camFront = glm::normalize(camFront);

            float moveSpeed{ deltaTime * m_movementSpeed };

            if(m_keys[Direction::UP])
                m_position += camFront * moveSpeed;
            if(m_keys[Direction::DOWN])
                m_position -= camFront * moveSpeed;
            if(m_keys[Direction::LEFT])
                m_position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            if(m_keys[Direction::RIGHT])
                m_position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
        }
    }
};

}  // namespace aph
