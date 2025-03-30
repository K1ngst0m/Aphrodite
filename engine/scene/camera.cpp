#include "camera.h"

namespace aph
{
Camera& Camera::setProjection(PerspectiveInfo perspective)
{
    m_cameraType = CameraType::Perspective;
    m_perspective = perspective;
    m_dirty.projection = true;
    return *this;
}

Camera& Camera::setProjection(OrthographicInfo orthographic)
{
    m_cameraType = CameraType::Orthographic;
    m_orthographic = orthographic;
    m_dirty.projection = true;
    return *this;
}

Camera& Camera::setLookAt(const Vec3& eye, const Vec3& at, const Vec3& up)
{
    m_position = Vec4(eye, 1.0f);
    // Since glm::quatLookAt isn't directly wrapped, we'll create a look-at rotation matrix
    // and convert it to a quaternion
    Mat4 lookMat = LookAt(eye, at, up);
    m_orientation = Mat4ToQuat(lookMat);
    
    m_dirty.view = true;
    return *this;
}

Camera& Camera::setPosition(Vec3 value)
{
    m_position = Vec4(value, 1.0f);
    m_dirty.view = true;
    return *this;
}

void Camera::updateProjection()
{
    switch (getType())
    {
    case CameraType::Orthographic:
    {
        setProjection(Ortho(m_orthographic.left, m_orthographic.right, 
                           m_orthographic.bottom, m_orthographic.top,
                           m_orthographic.znear, m_orthographic.zfar));
    }
    break;
    case CameraType::Perspective:
    {
        auto matrix = Perspective(Radians(m_perspective.fov), m_perspective.aspect, 
                                 m_perspective.znear, m_perspective.zfar);
        if (m_flipY)
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
    // Conjugate the quaternion (equivalent to inverse for unit quaternions)
    Quat conjugate = Quat(m_orientation.w, -m_orientation.x, -m_orientation.y, -m_orientation.z);
    Mat4 rot = QuatToMat4(conjugate);
    Mat4 trans = Translate(Mat4(1.0f), -Vec3(m_position));
    setView(rot * trans);
}

const Mat4& Camera::getProjection()
{
    if (m_dirty.projection)
    {
        updateProjection();
    }
    return m_projection;
}

const Mat4& Camera::getView()
{
    if (m_dirty.view)
    {
        updateView();
    }
    return m_view;
}

Camera& Camera::setProjection(Mat4 value)
{
    m_projection = value;
    m_dirty.projection = false;
    return *this;
}

Camera& Camera::setView(Mat4 value)
{
    m_view = value;
    m_dirty.view = false;
    return *this;
}
} // namespace aph
