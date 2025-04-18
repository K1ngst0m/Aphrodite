#pragma once

#include "math/math.h"

namespace aph
{
enum class CameraType
{
    Orthographic,
    Perspective,
};

struct OrthographicInfo
{
    float left   = {};
    float right  = {};
    float bottom = {};
    float top    = {};
    float znear  = { 1.0f };
    float zfar   = { 1000.0f };
};

struct PerspectiveInfo
{
    float aspect = { 16.0f / 9.0f };
    float fov    = { 60.0f };
    float znear  = { 1.0f };
    float zfar   = { 1000.0f };
};

class Camera
{
public:
    explicit Camera(CameraType cameraType);

    auto getType() const -> CameraType;
    auto getProjection() -> const Mat4&;
    auto getView() -> const Mat4&;
    auto getPerspectiveInfo() const -> const PerspectiveInfo&;
    auto getOrthographicInfo() const -> const OrthographicInfo&;

    auto setProjection(PerspectiveInfo perspective) -> Camera&;
    auto setProjection(OrthographicInfo orthographic) -> Camera&;
    auto setProjection(Mat4 value) -> Camera&;
    auto setLookAt(const Vec3& eye, const Vec3& at, const Vec3& up) -> Camera&;
    auto setView(Mat4 value) -> Camera&;
    auto setPosition(Vec3 value) -> Camera&;

    ~Camera() = default;

private:
    void updateProjection();
    void updateView();

private:
    CameraType m_cameraType{ CameraType::Perspective };

    Mat4 m_projection{ 1.0f };
    Mat4 m_view{ 1.0f };

    Vec4 m_position{ 0.0f };
    Quat m_orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

    bool m_flipY{ true };

    OrthographicInfo m_orthographic;
    PerspectiveInfo m_perspective;

    struct
    {
        bool projection = true;
        bool view       = true;
    } m_dirty;
};

} // namespace aph
