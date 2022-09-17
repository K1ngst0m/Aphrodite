#include "vklCamera.h"

namespace vkl {
Camera::Camera(float aspect, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_zoom(ZOOM),
      m_aspect(aspect) {
    m_position = position;
    m_worldUp  = up;
    m_yaw      = yaw;
    m_pitch    = pitch;
    updateCameraVectors();
}
Camera::Camera(float aspect, float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw,
               float pitch)
    : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_zoom(ZOOM),
      m_aspect(aspect) {
    m_position = glm::vec3(posX, posY, posZ);
    m_worldUp  = glm::vec3(upX, upY, upZ);
    m_yaw      = yaw;
    m_pitch    = pitch;
    updateCameraVectors();
}
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}
void Camera::move(CameraMoveDirection direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    if (direction == CameraMoveDirection::FORWARD)
        m_position += m_front * velocity;
    if (direction == CameraMoveDirection::BACKWARD)
        m_position -= m_front * velocity;
    if (direction == CameraMoveDirection::LEFT)
        m_position -= m_right * velocity;
    if (direction == CameraMoveDirection::RIGHT)
        m_position += m_right * velocity;
}
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        if (m_pitch > 89.0f)
            m_pitch = 89.0f;
        if (m_pitch < -89.0f)
            m_pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}
void Camera::ProcessMouseScroll(float yoffset) {
    m_zoom -= (float)yoffset;
    if (m_zoom < 1.0f)
        m_zoom = 1.0f;
    if (m_zoom > 45.0f)
        m_zoom = 45.0f;
}
void Camera::updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    m_right = glm::normalize(glm::cross(front,
                                        m_worldUp)); // normalize the vectors, because their length gets closer to 0 the
                                                     // more you look up or down which results in slower movement.
    m_up = glm::normalize(glm::cross(m_right, front));
}
glm::mat4 Camera::GetProjectionMatrix() const {
    auto result = glm::perspective(m_zoom, m_aspect, NEAR, FAR);
    result[1][1] *= -1;
    return result;
}
glm::mat4 Camera::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}
} // namespace vkl
