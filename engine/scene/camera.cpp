#include "camera.h"

namespace aph
{
Camera& Camera::setProjection(Perspective perspective)
{
    m_cameraType       = CameraType::Perspective;
    m_perspective      = perspective;
    m_dirty.projection = true;
    return *this;
}

Camera& Camera::setProjection(Orthographic orthographic)
{
    m_cameraType       = CameraType::Orthographic;
    m_orthographic     = orthographic;
    m_dirty.projection = true;
    return *this;
}

Camera& Camera::setLookAt(const glm::vec3& eye, const glm::vec3& at, const glm::vec3& up)
{
    m_position        = glm::vec4(eye, 1.0f);
    glm::vec3 forward = glm::normalize(at - eye);
    m_orientation     = glm::quatLookAt(forward, glm::normalize(up));
    m_dirty.view      = true;
    return *this;
}

Camera& Camera::setPosition(glm::vec4 value)
{
    m_position   = value;
    m_dirty.view = true;
    return *this;
}

void Camera::updateProjection()
{
    switch(getType())
    {
    case CameraType::Orthographic:
    {
        setProjection(glm::ortho(m_orthographic.left, m_orthographic.right, m_orthographic.bottom, m_orthographic.top,
                                 m_orthographic.znear, m_orthographic.zfar));
    }
    break;
    case CameraType::Perspective:
    {
        auto matrix = glm::perspective(glm::radians(m_perspective.fov), m_perspective.aspect, m_perspective.znear,
                                       m_perspective.zfar);
        if(m_flipY)
        {
            matrix[1][1] *= -1.0f;
        }
        setProjection(matrix);
    }
    break;
    }
}

void Camera::updateView()
{
    glm::mat4 rot   = glm::mat4_cast(glm::conjugate(m_orientation));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), -glm::vec3(m_position));
    setView(rot * trans);
}

glm::mat4 Camera::getProjection()
{
    if(m_dirty.projection)
    {
        updateProjection();
    }
    return m_projection;
}

glm::mat4 Camera::getView()
{
    if(m_dirty.view)
    {
        updateView();
    }
    return m_view;
}

Camera& Camera::setProjection(glm::mat4 value)
{
    m_projection       = value;
    m_dirty.projection = false;
    return *this;
}

Camera& Camera::setView(glm::mat4 value)
{
    m_view       = value;
    m_dirty.view = false;
    return *this;
}
}  // namespace aph

