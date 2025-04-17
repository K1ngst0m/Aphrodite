#include "camera.h"

namespace aph
{
auto Camera::setProjection(PerspectiveInfo perspective) -> Camera&
{
    m_cameraType       = CameraType::Perspective;
    m_perspective      = perspective;
    m_dirty.projection = true;
    return *this;
}

auto Camera::setProjection(OrthographicInfo orthographic) -> Camera&
{
    m_cameraType       = CameraType::Orthographic;
    m_orthographic     = orthographic;
    m_dirty.projection = true;
    return *this;
}

auto Camera::setLookAt(const Vec3& eye, const Vec3& at, const Vec3& up) -> Camera&
{
    m_position = Vec4(eye, 1.0f);
    // and convert it to a quaternion
    Mat4 lookMat  = LookAtLH(eye, at, up);
    m_orientation = Mat4ToQuat(lookMat);

    m_dirty.view = true;
    return *this;
}

auto Camera::setPosition(Vec3 value) -> Camera&
{
    m_position   = Vec4(value, 1.0f);
    m_dirty.view = true;
    return *this;
}

void Camera::updateProjection()
{
    switch (getType())
    {
    case CameraType::Orthographic:
    {
        setProjection(OrthoLH(m_orthographic.left, m_orthographic.right, m_orthographic.bottom, m_orthographic.top,
                              m_orthographic.znear, m_orthographic.zfar));
    }
    break;
    case CameraType::Perspective:
    {
        auto matrix =
            PerspectiveLH(Radians(m_perspective.fov), m_perspective.aspect, m_perspective.znear, m_perspective.zfar);
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
    Mat4 rot       = QuatToMat4(conjugate);
    Mat4 trans     = Translate(Mat4(1.0f), -Vec3(m_position));
    setView(rot * trans);
}

auto Camera::getProjection() -> const Mat4&
{
    if (m_dirty.projection)
    {
        updateProjection();
    }
    return m_projection;
}

auto Camera::getView() -> const Mat4&
{
    if (m_dirty.view)
    {
        updateView();
    }
    return m_view;
}

auto Camera::setProjection(Mat4 value) -> Camera&
{
    m_projection       = value;
    m_dirty.projection = false;
    return *this;
}

auto Camera::setView(Mat4 value) -> Camera&
{
    m_view       = value;
    m_dirty.view = false;
    return *this;
}

Camera::Camera(CameraType cameraType)
    : m_cameraType(cameraType)
{
}

auto Camera::getType() const -> CameraType
{
    return m_cameraType;
}

auto Camera::getPerspectiveInfo() const -> const PerspectiveInfo&
{
    return m_perspective;
}

auto Camera::getOrthographicInfo() const -> const OrthographicInfo&
{
    return m_orthographic;
}
} // namespace aph
